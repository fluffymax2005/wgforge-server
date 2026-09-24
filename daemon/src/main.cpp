/*
 * main.cpp
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

#include <sqlite3.h>

#include <exception>
#include <format>
#include <iostream>
#include <wginterface.hpp>

#include "config_error_handler.h"
#include "config_manager.h"
#include "handlers.h"
#include "initializer.h"
#include "request_handler.h"
#include "sqlite_db_manager.h"
#include "sqlite_error_handler.h"
#include "wgforgeserver.h"

// Create phony config for tests
#define __PHONY_CONFIG__

int main(int argc, char* argv[]) {
  // Check command line arguments.
  if (argc != 2) {
	std::cerr << "Usage: wgforgeserver <interface_name>\n";
	std::cerr << "  For example, try:\n";
	std::cerr << "   " << argv[0] << " wg0\n";
	return 1;
  }

  try {
	WgForge::log_info("Loading config or creating if it does not exist");

	// Config
	WgForge::ConfigManager config_manager;

	const std::string conf_path(std::format("/etc/wgforge/{}.conf", argv[1]));
	config_manager.open(conf_path, false);

	WgForge::ConfigManager::ConfigReturnCode cfg_rc{};
	if (!config_manager.isOpen())
	  cfg_rc = config_manager.create(conf_path, WgForge::create_config);

	WgForge::ConfigErrorHandler::throwIfError(cfg_rc);

	// SQLite3 DB
	WgForge::log_info("Configuring SQLite Database");

	const std::string db_path(std::format("/var/lib/wgforge/{}.sqlite", argv[1]));

	WgForge::SQLiteDbManager db_manager;
	WgForge::SQLiteDbManager::SQLiteReturnCode db_rc = db_manager.openConnection(db_path, false);

	WgForge::SQLiteErrorHandler::throwIfError(db_rc);
	WgForge::setup_database(db_manager);

	// Wireguard iface

	WgForge::log_info("Configuring Wireguard interface");

	WgForge::WgInterface iface = WgForge::setup_wginterface(config_manager, db_manager, argv[1]);

	WgForge::log_info("Configuring HTTP server endpoints");

	const auto srvCfg = WgForge::get_server_config(config_manager);
	const auto ifaceCfg = WgForge::get_iface_config(config_manager, argv[1]);
	const auto wireguardCfg = WgForge::get_wireguard_config(config_manager);

	WgForge::EndpointArgs args{iface, config_manager, db_manager, srvCfg, ifaceCfg, wireguardCfg};
	WgForge::RequestHandler handler;
	WgForge::setup_endpoints(args, handler);

	WgForge::log_info("Configuring HTTP server");

	// HTTP server
	WgForge::WgForgeServer s = WgForge::setup_wgforgeserver(config_manager);
	s.setHandler(std::move(handler));


	WgForge::log_info("HTTP server is listening on ", srvCfg.addr, ':', srvCfg.listenPort);
	WgForge::log_info("Wireguard VPN tunnel is listening on 0.0.0.0:", ifaceCfg.listenPort,
	                  ". Proto: ", ifaceCfg.proto_type == Protocol::IPv4 ? "IPv4" : "IPv6",
	                  ". VPN tunnel address: ", ifaceCfg.addr);

	// Run the server until stopped.
	s.run();

  } catch (std::exception& e) {
	WgForge::log_error(e.what());
	return 1;
  }

  return 0;
}
