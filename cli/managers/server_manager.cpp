/*
 * server_manager.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#include "server_manager.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdexcept>

namespace WgForge {

ServerManager::ServerManager(std::shared_ptr<ConfigManager> cfg_mgr) {
  if (cfg_mgr == nullptr)
	throw std::invalid_argument("Config manager is nullptr");
  cfg_mgr_ = std::move(cfg_mgr);
}

ServerConfig ServerManager::getConfig(const std::string& iface) const {
  if (iface.empty())
	throw std::invalid_argument("Enter interface name");
  if (!ensureConfigOpenedImpl(iface))
	throw std::runtime_error("Failed to open config to read server details");
  const std::string path = std::format("/etc/wgforge/{}.conf", iface);
  ServerConfig config;

  ConfigReadRegister::ReadReturnCode rc;
  auto addr_opt = cfg_mgr_->tryRead<std::string>("Server", "Address", &rc);
  if (rc)
	throw std::runtime_error(
	    std::format("Failed to read server configuration from \"{}\". Error code = {}. Try check "
		            "it's existance and "
		            "rights to read as well as mandatory fields",
		            path, rc));
  std::strcpy(config.addr, addr_opt->c_str());

  auto port_opt = cfg_mgr_->tryRead<uint32_t>("Server", "ListenPort", &rc);
  if (rc)
	throw std::runtime_error(
	    std::format("Failed to read server configuration from \"{}\". Error code = {}. Try check "
		            "it's existance and "
		            "rights to read as well as mandatory fields",
		            path, rc));
  config.listenPort = *port_opt;

  return config;
}

void ServerManager::updateConfig(const UpdateServerRequest& req) {
  if (!req.requiresQuery())
	throw std::invalid_argument("No data to update server config provided");
  if (!ensureConfigOpenedImpl(req.iface, false))
	throw std::runtime_error("Failed to open config to read server details");


  const std::string path = std::format("/etc/wgforge/{}.conf", req.iface);
  cfg_mgr_->beginTransaction();
  if (req.address) {
	in_addr addr_bin;
	if (inet_pton(AF_INET, req.address->c_str(), &addr_bin) != 0) {
	  cfg_mgr_->commit();
	  throw std::invalid_argument("Invalid address");
	}
	try {
	  cfg_mgr_->write("Server", "Address", *req.address);
	} catch (...) {
	  cfg_mgr_->commit();
	  throw std::runtime_error("Failed update to server's configuration field: Address");
	}
  }
  if (req.listen_port) {
	try {
	  cfg_mgr_->write("Server", "ListenPort", *req.listen_port);
	} catch (...) {
	  cfg_mgr_->commit();
	  throw std::runtime_error("Failed update to server's configuration field: ListenPort");
	}
  }

  cfg_mgr_->commit();
}

bool ServerManager::ensureConfigOpenedImpl(const std::string& iface, bool readonly) const {
  const auto path = std::format("/etc/wgforge/{}.conf", iface);
  if (cfg_mgr_->path() == path)
	return true;

  cfg_mgr_->close();
  if (cfg_mgr_->open(path, readonly))
	return false;
  return true;
}

}  // namespace WgForge
