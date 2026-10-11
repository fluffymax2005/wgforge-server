/*
 * interface_controller.cpp
 * wgforge-server — Backend to manage Wireguard
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#include "wireguard_interface_controller.h"

namespace WgForge {

WireguardInterfaceController::WireguardInterfaceController(
    std::shared_ptr<ILogger> logger, std::shared_ptr<ConfigManager> config_mgr) {
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");
  if (config_mgr == nullptr)
	throw std::invalid_argument("Config manager is nullptr");

  logger_ = std::move(logger);
  config_mgr_ = std::move(config_mgr);
}

void WireguardInterfaceController::getInfo(Request req, Response res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& iface = args.iface;

  constexpr const char* URI = "/interface";
  constexpr const char* METHOD = "GET";
  std::exception_ptr ep;

  try {
	// Iface address
	nl_sock* sock = nl_socket_alloc();
	if (sock == nullptr) {
	  throw EndpointRuntimeError("Failed to alloc netlink socket", URI, METHOD);
	}

	auto ec = nl_connect(sock, NETLINK_ROUTE);
	if (ec < 0) {
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to connect netlink socket", URI, METHOD);
	}

	nl_cache* link_cache;
	if ((ec = rtnl_link_alloc_cache(sock, AF_INET, &link_cache)) < 0) {
	  nl_close(sock);
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to alloc rtnl link cache", URI, METHOD);
	}

	rtnl_link* link = rtnl_link_get_by_name(link_cache, iface.getName());
	if (link == nullptr) {
	  nl_cache_free(link_cache);
	  nl_close(sock);
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to get link by name", URI, METHOD);
	}

	nl_cache* addr_cache;
	if ((ec = rtnl_addr_alloc_cache(sock, &addr_cache)) < 0) {
	  rtnl_link_put(link);
	  nl_cache_free(link_cache);
	  nl_close(sock);
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to alloc rtnl address cache", URI, METHOD);
	}

	auto ifaceIndex = rtnl_link_get_ifindex(link);
	char ip_addr[INET_ADDRSTRLEN]{'\0'};

	nl_object* object = nl_cache_get_first(addr_cache);
	while (object) {
	  rtnl_addr* addr = reinterpret_cast<rtnl_addr*>(object);
	  if (rtnl_addr_get_ifindex(addr) == ifaceIndex) {
		nl_addr* local = rtnl_addr_get_local(addr);
		if (local) {
		  nl_addr2str(local, ip_addr, sizeof(ip_addr));
		  break;
		}
	  }

	  object = nl_cache_get_next(object);
	}

	nl_cache_free(addr_cache);
	rtnl_link_put(link);
	nl_cache_free(link_cache);
	nl_close(sock);
	nl_socket_free(sock);

	// Failed to found ip address
	if (ip_addr[0] == '\0') {
	  throw EndpointRuntimeError("Failed to find any IPv4 address corresponding to interface", URI,
	                             METHOD);
	}

	json::object obj;
	obj["ifaceName"] = iface.getName();
	obj["isUp"] = iface.isUp();
	obj["listenPort"] = iface.getPort();
	obj["ipAddress"] = ip_addr;
	obj["privateKey"] = iface.getPrivateKey()->toB64String();
	obj["publicKey"] = iface.getPublicKey()->toB64String();

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(obj);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

void WireguardInterfaceController::updateConfig(Request req, Response res) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "ONLY PATCH";
	res.prepare_payload();
	return;
  }

  auto& config = args.config;
  constexpr const char* URI = "/interface";
  constexpr const char* METHOD = "PATCH";
  std::exception_ptr ep;

  try {
	auto j = json::parse(req.body()).as_object();

	auto key_opt = get_optional_field<std::string_view>(j, "privateKey", false);
	if (key_opt && !WgKey::validateB64StringKey(*key_opt))
	  throw EndpointInvalidArgument("Invalid private key");

	auto addr_opt = get_optional_field<std::string_view>(j, "address", false);
	if (addr_opt && !WgAllowedIP::validate(addr_opt->data()))
	  throw EndpointInvalidArgument("Invalid listen address");

	auto port_opt = get_optional_field<int64_t>(j, "listenPort", false);
	if (port_opt && port_opt.value() > std::numeric_limits<in_port_t>::max())
	  throw EndpointInvalidArgument("Invalid listen port");

	ConfigManager::ConfigReturnCode rc = config.beginTransaction();
	if (rc)
	  throw EndpointRuntimeError("Failed to start transaction", URI, METHOD);

	if (key_opt && (rc = config.write("Interface", "PrivateKey", key_opt->data())))
	  throw EndpointRuntimeError(
	      ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"), URI, METHOD);

	if (addr_opt && (rc = config.write("Interface", "Address", addr_opt->data()))) {
	  config.rollBack();
	  throw EndpointRuntimeError(
	      ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"), URI, METHOD);
	}

	if (port_opt && (rc = config.write("Interface", "ListenPort", port_opt.value()))) {
	  config.rollBack();
	  throw EndpointRuntimeError(
	      ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"), URI, METHOD);
	}

	config.commit();

	res.result(http::status::no_content);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

}  // namespace WgForge
