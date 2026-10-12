/*
 * initializer.cpp
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

#include "initializer.h"

#include <arpa/inet.h>
#include <curl/curl.h>
#include <libeconf.h>
#include <sqlite3.h>

#include <filesystem>
#include <format>
#include <odb/query.hxx>
#include <unordered_set>

#include "config_error_handler.h"
#include "config_read_write_register.h"
#include "config_write_functions.h"
#include "config_write_register.h"
#include "inetutils.h"
#include "peer-odb.hxx"
#include "subnetscanner.h"
#include "wgforge.h"

namespace WgForge {




void setup_wginterface(WgInterface& iface, odb::database& context, const InterfaceConfig& config) {
  if (config.checkSubnet && !SubnetScanner::isSubnetFree(config.addr))
	throw std::invalid_argument(std::format("Provided subnet \"{}\" contains host", config.addr));

  odb::transaction t(context.begin());
  auto peers_query = context.query<Peer>(odb::query<Peer>::deleted_at.is_null() &&
                                         odb::query<Peer>::status == Peer::Status::Active);
  if (!peers_query.empty()) {
	for (const auto& peer : peers_query) {
	  WgPeer wgpeer;

	  try {
		wgpeer.setPublicKey(WgPublicKey(peer.public_key));
		wgpeer.setPersistentKeepAlive(peer.persistent_keepalive);
		if (peer.preshared_key)
		  wgpeer.setPresharedKey(WgPresharedKey(peer.preshared_key.value()));
		wgpeer.addAllowedIP(WgAllowedIP(peer.tunnel_address));

		iface.addPeer(std::move(wgpeer));
	  } catch (const std::exception&) {
		throw std::runtime_error(std::format(
		    "Failed to apply changes to kernel for peer with public key: {}", peer.public_key));
	  }
	}
  }
  t.commit();

  iface.setListenPort(config.listenPort);
  iface.setPrivateKey(WgPrivateKey(config.private_key.toB64String()));
  iface.set();

  InetUtils::assignIpAddress(config);

  iface.bringUp();
}

econf_file* create_config(std::string_view path, ConfigManager::ConfigReturnCode& rc) noexcept {
  // Initialize new config
  econf_file* config;
  econf_err ec = econf_newKeyFile(&config, '=', '#');
  if (ec != ECONF_SUCCESS) {
	rc = static_cast<ConfigManager::ConfigReturnCode>(ec);
	return nullptr;
  }

  try {
	ConfigReadWriteRegister rw_reg;
	rw_reg.registerWriter<int>(LibeconfWriters::writeInt);
	rw_reg.registerWriter<std::string>(LibeconfWriters::writeString);
	rw_reg.registerWriter<bool>(LibeconfWriters::writeBool);

	ConfigManager cfg(config, false);
	cfg.setReadWriteRegister(std::move(rw_reg));

	// Iterate through /etc/wgforge and extract all occupied ports for other ifaces.
	std::unordered_set<in_port_t> occupied_ports;
	try {
	  namespace fs = std::filesystem;
	  constexpr std::string_view conf_dir = "/etc/wgforge";
	  if (fs::exists(conf_dir) && fs::is_directory(conf_dir)) {
		for (const auto& entry : fs::directory_iterator(conf_dir)) {
		  if (entry.is_regular_file() && entry.path().extension() == ".conf") {
			ConfigManager existing_config;
			if (existing_config.open(entry.path().c_str()))
			  continue;
			auto port_opt = existing_config.tryRead<int64_t>("Interface", "ListenPort");
			if (port_opt)
			  occupied_ports.insert(*port_opt);
		  }
		}
	  }
	} catch (...) {
	}

	// Iterate through available port and find first free one.
	int listen_port = -1;
	for (auto port = 51820; port <= std::numeric_limits<in_port_t>::max(); port++) {
	  if (occupied_ports.contains(port))
		continue;
	  try {
		if (InetUtils::isPortFree("0.0.0.0", port, false)) {
		  listen_port = port;
		  break;
		}
	  } catch (...) {
	  }
	}

	if (listen_port == -1) {
	  rc = WGFORGE_CONFIG_NOMEM;
	  return nullptr;
	}

	// [Interface]
	// ListenPort=51820
	if ((rc = cfg.write<int>("Interface", "ListenPort", listen_port)))
	  return nullptr;

	// [Interface]
	// PrivateKey=<private_key>
	const std::string private_key = WgPrivateKey().toB64String();
	if ((rc = cfg.write<std::string>("Interface", "PrivateKey", private_key)))
	  return nullptr;

	// [Interface]
	// Address=<cidr> Default suffix = 24
	const auto net_cidr = SubnetScanner::findFreeSubnet(24);
	const auto iface_cidr = InetUtils::getGatewayCIDR(net_cidr);
	if ((rc = cfg.write<std::string>("Interface", "Address", iface_cidr)))
	  return nullptr;

	// [Interface]
	// CheckSubnet=<true_or_false>
	// Should programm check subnet for WgInterface before start?
	// Default is true
	if ((rc = cfg.write<bool>("Interface", "CheckSubnet", true)))
	  return nullptr;

	// [Server]
	// Address=<local_listen_address>
	const auto srv_address = InetUtils::findFreeLoopbackAddress(51819);
	if ((rc = cfg.write<std::string>("Server", "Address", srv_address)))
	  return nullptr;

	// [Server]
	// ListenPort=51819
	if ((rc = cfg.write<int>("Server", "ListenPort", 51819)))
	  return nullptr;

	// [Wireguard]
	// PublicEndpoint=<public_endpoint_of_wireguard_server>
	// By default <public_ip>:<wireguard_listen_port> by default
	const auto server_public_ip_opt = __get_public_ip();
	if (!server_public_ip_opt) {
	  if ((rc = cfg.write<std::string>("Wireguard", "PublicEndpoint",
	                                   std::format("<PublicIpAddress>:{}", listen_port))))
		return nullptr;
	} else {
	  if ((rc = cfg.write<std::string>("Wireguard", "PublicEndpoint",
	                                   std::format("{}:{}", *server_public_ip_opt, listen_port))))
		return nullptr;
	}

	// Write file
	char buf[128];
	const auto iface_start_index = path.find_last_of('/');
	path.copy(buf, iface_start_index);
	buf[iface_start_index] = '\0';  // directory path

	if ((ec = econf_writeFile(config, buf, path.data() + iface_start_index + 1)) != ECONF_SUCCESS)
	  return nullptr;

	cfg.release();

  } catch (const std::bad_alloc&) {
	rc = WGFORGE_CONFIG_NOMEM;
	config = nullptr;
  }

  rc = ECONF_SUCCESS;
  return config;
}  // namespace WgForge

InterfaceConfig get_iface_config(const ConfigManager& config, const char* name) {
  InterfaceConfig ifaceCfg;

  // Extract iface name from config filepath
  std::strncpy(ifaceCfg.name, name, sizeof(ifaceCfg.name));

  // ListenPort
  ConfigReadRegister::ReadReturnCode rc;
  if (auto opt = config.tryRead<int>("Interface", "ListenPort", &rc))
	ifaceCfg.listenPort = opt.value();
  else
	throw std::runtime_error(ConfigErrorHandler::readToString(rc, "Interface", "ListenPort"));

  if (ifaceCfg.listenPort < 0 ||
      ifaceCfg.listenPort > std::numeric_limits<decltype(InterfaceConfig::listenPort)>::max())
	throw std::runtime_error("Invalid listen port number provided for Wireguard interface");

  // Address
  if (auto opt = config.tryRead<std::string>("Interface", "Address", &rc))
	std::strncpy(ifaceCfg.addr, opt->c_str(), sizeof(ifaceCfg.addr));
  else
	throw std::runtime_error(ConfigErrorHandler::readToString(rc, "Interface", "Address"));

  try {
	WgAllowedIP allowed_ip{ifaceCfg.addr};
	ifaceCfg.proto_type = allowed_ip.getProto();
  } catch (...) {
	throw std::invalid_argument("Invalid CIDR for \"Address\" provided by [Interface]");
  }

  // Private key
  if (auto opt = config.tryRead<std::string>("Interface", "PrivateKey", &rc)) {
	if (!WgKey::validateB64StringKey(opt->c_str()))
	  throw std::invalid_argument("Inappropriate private key supplied");
	ifaceCfg.private_key = WgPrivateKey(opt->c_str());
  } else
	throw std::runtime_error(ConfigErrorHandler::readToString(rc, "Interface", "PrivateKey"));

  // CheckSubnet
  if (auto opt = config.tryRead<bool>("Interface", "CheckSubnet", &rc))
	ifaceCfg.checkSubnet = *opt;

  return ifaceCfg;
}

ServerConfig get_server_config(const ConfigManager& config) {
  ServerConfig srvCfg;

  ConfigReadRegister::ReadReturnCode rc;
  if (auto opt = config.tryRead<std::string>("Server", "Address", &rc)) {
	char buf[sizeof(in_addr)];
	if (inet_pton(AF_INET, opt->c_str(), buf) != 1)
	  throw std::invalid_argument("Invalid \"Address\" provided for [Server]");
	std::strcpy(srvCfg.addr, opt->c_str());
  } else {
	throw std::runtime_error(ConfigErrorHandler::readToString(rc, "Server", "Address"));
  }

  if (auto opt = config.tryRead<int32_t>("Server", "ListenPort", &rc))
	srvCfg.listenPort = opt.value();
  else
	throw std::runtime_error(ConfigErrorHandler::readToString(rc, "Server", "ListenPort"));

  if (srvCfg.listenPort < 0 ||
      srvCfg.listenPort > std::numeric_limits<decltype(ServerConfig::listenPort)>::max())
	throw std::runtime_error("\"ListenPort\" out of range for [Server]");

  return srvCfg;
}

WireguardConfig get_wireguard_config(const ConfigManager& config) {
  WireguardConfig wireguardCfg;
  ConfigReadRegister::ReadReturnCode rc;
  if (auto opt = config.tryRead<std::string>("Wireguard", "PublicEndpoint", &rc)) {
	if (!WgEndpoint::validate(*opt))
	  throw std::runtime_error("Invalid \"PublicEndpoint\" in [Wireguard]");
	std::sprintf(wireguardCfg.public_endpoint, "%s", opt->c_str());
  } else {
	throw std::runtime_error(ConfigErrorHandler::readToString(rc, "Wireguard", "PublicEndpoint"));
  }

  return wireguardCfg;
}

static std::optional<std::string> __get_public_ip() {
  CURL* curl = curl_easy_init();
  if (curl == nullptr)
	return std::nullopt;
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> pcurl(curl, &curl_easy_cleanup);

  std::string ip;

  curl_easy_setopt(pcurl.get(), CURLOPT_URL, "https://api.ipify.org");
  curl_easy_setopt(
      pcurl.get(), CURLOPT_WRITEFUNCTION,
      +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
	    auto* s = static_cast<std::string*>(userdata);
	    s->append(ptr, size * nmemb);
	    return size * nmemb;
      });
  curl_easy_setopt(pcurl.get(), CURLOPT_WRITEDATA, &ip);
  curl_easy_setopt(pcurl.get(), CURLOPT_TIMEOUT, 5L);
  curl_easy_setopt(pcurl.get(), CURLOPT_USERAGENT, "wgforge/1.0");

  CURLcode rc = curl_easy_perform(pcurl.get());
  if (rc != CURLE_OK)
	return std::nullopt;
  return ip;
}

}  // namespace WgForge
