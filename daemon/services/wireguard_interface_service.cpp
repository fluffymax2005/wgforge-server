/*
 * wireguard_interface_service.cpp
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

#include "wireguard_interface_service.h"

#include <netlink/handlers.h>
#include <netlink/netlink.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/socket.h>

#include <stdexcept>

#include "config_error_handler.h"

namespace WgForge {

WireguardInterfaceService::WireguardInterfaceService(std::shared_ptr<ILogger> logger,
                                                     std::shared_ptr<ConfigManager> config_mgr,
                                                     std::shared_ptr<WgInterface> iface) {
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");
  if (config_mgr == nullptr)
	throw std::invalid_argument("Config manager is nullptr");
  if (iface == nullptr)
	throw std::invalid_argument("Interface is nullptr");

  logger_ = std::move(logger);
  config_mgr_ = std::move(config_mgr);
  iface_ = std::move(iface);
}

InterfaceInfoResponse WireguardInterfaceService::getInfo() const {
  // Iface address
  nl_sock* sock = nl_socket_alloc();
  if (sock == nullptr) {
	throw std::runtime_error("Failed to alloc netlink socket");
  }

  auto ec = nl_connect(sock, NETLINK_ROUTE);
  if (ec < 0) {
	nl_socket_free(sock);

	throw std::runtime_error("Failed to connect netlink socket");
  }

  nl_cache* link_cache;
  if ((ec = rtnl_link_alloc_cache(sock, AF_INET, &link_cache)) < 0) {
	nl_close(sock);
	nl_socket_free(sock);

	throw std::runtime_error("Failed to alloc rtnl link cache");
  }

  rtnl_link* link = rtnl_link_get_by_name(link_cache, iface_->getName());
  if (link == nullptr) {
	nl_cache_free(link_cache);
	nl_close(sock);
	nl_socket_free(sock);

	throw std::runtime_error("Failed to get link by name");
  }

  nl_cache* addr_cache;
  if ((ec = rtnl_addr_alloc_cache(sock, &addr_cache)) < 0) {
	rtnl_link_put(link);
	nl_cache_free(link_cache);
	nl_close(sock);
	nl_socket_free(sock);

	throw std::runtime_error("Failed to alloc rtnl address cache");
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
	throw std::runtime_error("Failed to find any IPv4 address corresponding to interface");
  }

  InterfaceInfoResponse res;
  res.iface_name = iface_->getName();
  res.is_up = iface_->isUp();
  res.listen_port = iface_->getPort();
  res.address = ip_addr;
  res.private_key = iface_->getPrivateKey()->toB64String();
  res.public_key = iface_->getPublicKey()->toB64String();

  return res;
}

void WireguardInterfaceService::updateConfig(const InterfaceUpdateRequest& req) {
  ConfigManager::ConfigReturnCode rc = config_mgr_->beginTransaction();
  if (rc)
	throw std::runtime_error("Failed to start transaction");

  if (req.private_key &&
      (rc = config_mgr_->write("Interface", "PrivateKey", req.private_key->c_str())))
	throw std::runtime_error(
	    ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"));

  if (req.address && (rc = config_mgr_->write("Interface", "Address", req.address->c_str()))) {
	config_mgr_->rollBack();
	throw std::runtime_error(
	    ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"));
  }

  if (req.listen_port &&
      (rc = config_mgr_->write("Interface", "ListenPort", req.listen_port.value()))) {
	config_mgr_->rollBack();
	throw std::runtime_error(
	    ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"));
  }

  config_mgr_->commit();
}


}  // namespace WgForge
