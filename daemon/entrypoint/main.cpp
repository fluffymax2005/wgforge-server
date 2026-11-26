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

#include <config_error_handler.h>

#include <boost/beast/http/verb.hpp>
#include <boost/di.hpp>
#include <config_manager.hpp>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <meta-odb.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>
#include <string>

#include "configuration_root.h"
#include "ilogger.h"
#include "initializer.h"
#include "ipeerservice.h"
#include "peer_controller.h"
#include "peer_expiry_scheduler.h"
#include "peer_service.h"
#include "request_handler.h"
#include "spdlogger.h"
#include "wgforgeserver.h"
#include "wireguard_interface_controller.h"
#include "wireguard_interface_service.h"

namespace di = boost::di;
namespace http = boost::beast::http;

int main(int argc, char* argv[]) {
  if (argc != 2) {
	std::cerr << "Usage: wgforgeserver <interface_name>\n";
	std::cerr << "  For example: " << argv[0] << " wg0\n";
	return 1;
  }

  const std::string iface_name = argv[1];

  std::shared_ptr<WgForge::ILogger> spd_logger;

  try {
	// Filepaths
	const std::string conf_path = std::format("/etc/wgforge/{}.conf", iface_name);
	const std::string db_path = std::format("/var/lib/wgforge/{}.sqlite", iface_name);

	// Logger
	spd_logger = std::make_shared<WgForge::SpdLogger>();
	spd_logger->info("Starting wgforge-server initialization...");

	// Configuration files (.conf)
	spd_logger->info("Loading configuration file: " + conf_path);
	auto config_manager = std::make_shared<WgForge::ConfigManager>();
	config_manager->open(conf_path, false);

	WgForge::ConfigManager::ConfigReturnCode cfg_rc{};
	if (!config_manager->isOpen()) {
	  cfg_rc = config_manager->create(conf_path, WgForge::create_config);
	}
	WgForge::ConfigErrorHandler::throwIfError(cfg_rc);

	// IConfiguration
	const auto IFACE_CFG = WgForge::get_iface_config(*config_manager, iface_name.c_str());
	auto configuration = std::make_shared<WgForge::Configuration>();
	configuration->getSection("Interface")["Address"] = IFACE_CFG.addr;
	configuration->getSection("Interface")["ListenPort"] = IFACE_CFG.listenPort;
	configuration->getSection("Interface")["PrivateKey"] = IFACE_CFG.private_key.toB64String();
	configuration->getSection("Interface")["CheckSubnet"] = IFACE_CFG.checkSubnet;
	configuration->getSection("Interface")["Name"] = IFACE_CFG.name;
	configuration->getSection("Interface")["Protocol"] =
	    IFACE_CFG.proto_type == Protocol::IPv4 ? "IPv4" : "IPv6";

	const auto SRV_CFG = WgForge::get_server_config(*config_manager);
	configuration->getSection("Server")["Address"] = SRV_CFG.addr;
	configuration->getSection("Server")["ListenPort"] = SRV_CFG.listenPort;
	configuration->getSection("Server")["AllowSubnetMigration"] = SRV_CFG.allow_subnet_migration;
	configuration->getSection("Server")["PeerExpireUpdate"] = SRV_CFG.peer_expire_update.count();

	const auto WG_CONFIG = WgForge::get_wireguard_config(*config_manager);
	configuration->getSection("Wireguard")["PublicEndpoint"] = WG_CONFIG.public_endpoint;

	auto create_db_context = [&]() -> std::shared_ptr<odb::sqlite::database> {
	  auto context = std::make_shared<odb::sqlite::database>(
	      db_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
	  odb::transaction t(context->begin());
	  if (odb::schema_catalog::exists(*context)) {
		try {
		  odb::schema_catalog::create_schema(*context, "", false);
		} catch (...) {
		}
	  }

	  auto res = context->query<WgForge::Meta>();
	  if (res.empty()) {
		WgForge::Meta meta_inf{.iface_address = IFACE_CFG.addr};
		context->persist(meta_inf);
	  }

	  t.commit();
	  return context;
	};

	// ODB (SQLite)
	spd_logger->info("Opening SQLite database: " + db_path);
	auto db_context = create_db_context();
	if (!db_context)
	  throw std::runtime_error("Failed to open database connection");

	spd_logger->info("Validating configuration and database containment");
	auto need_migration = WgForge::validateDatabaseSchemaUseCase(*config_manager, *db_context);
	if (need_migration) {
	  spd_logger->warn(
	      "Configuration and database differs from each other. Creating fresh database");

	  // Close previous connection
	  db_context.reset();

	  auto backup_path = WgForge::migrateDatabase(iface_name);
	  spd_logger->info(
	      std::format("Migration succeeded. Database was moved by path \"{}\"", backup_path));


	  db_context = create_db_context();
	  if (!db_context)
		throw std::runtime_error("Failed to create fresh database");
	}

	// Wireguard iface
	spd_logger->info("Binding Wireguard interface: " + iface_name);
	auto wg_interface = std::make_shared<WgForge::WgInterface>(iface_name.c_str());

	WgForge::setup_wginterface(*wg_interface, *db_context, IFACE_CFG);

	// Dependency Injection
	spd_logger->info("Building Dependency Injection Graph...");

	const auto injector = di::make_injector(
	    // Bind to instances
	    di::bind<odb::database>().to(db_context), di::bind<WgForge::WgInterface>().to(wg_interface),
	    di::bind<WgForge::ConfigManager>().to(config_manager),
	    di::bind<WgForge::IConfiguration>().to(configuration),

	    // Logger as Singleton
	    di::bind<WgForge::ILogger>().to(spd_logger),

	    di::bind<WgForge::IPeerServiceAsync>().to<WgForge::PeerService>(),
	    di::bind<WgForge::IInterfaceService>().to<WgForge::WireguardInterfaceService>(),

	    // Controllers
	    di::bind<WgForge::PeerController>().to<WgForge::PeerController>(),
	    di::bind<WgForge::WireguardInterfaceController>()
	        .to<WgForge::WireguardInterfaceController>());

	// Resolving controllers and binding routes
	spd_logger->info("Resolving Controllers and binding HTTP routes...");

	auto peer_controller = injector.create<std::shared_ptr<WgForge::PeerController>>();
	auto iface_controller =
	    injector.create<std::shared_ptr<WgForge::WireguardInterfaceController>>();

	WgForge::RequestHandler request_handler;

	// PeerController routes
	request_handler.bind_route(http::verb::get, "/peer", peer_controller,
	                           &WgForge::PeerController::getPeer);
	request_handler.bind_route(http::verb::get, "/peers", peer_controller,
	                           &WgForge::PeerController::getAllPeers);
	request_handler.bind_route(http::verb::post, "/peer", peer_controller,
	                           &WgForge::PeerController::createPeer);
	request_handler.bind_route(http::verb::patch, "/peer", peer_controller,
	                           &WgForge::PeerController::updatePeer);
	request_handler.bind_route(http::verb::delete_, "/peer", peer_controller,
	                           &WgForge::PeerController::deletePeer);
	request_handler.bind_route(http::verb::patch, "/peer/rotate", peer_controller,
	                           &WgForge::PeerController::rotatePeerKey);
	request_handler.bind_route(http::verb::get, "/peer/config", peer_controller,
	                           &WgForge::PeerController::getPeersConfig);

	// WireguardInterfaceController routes
	request_handler.bind_route(http::verb::get, "/interface", iface_controller,
	                           &WgForge::WireguardInterfaceController::getInfo);
	request_handler.bind_route(http::verb::patch, "/interface", iface_controller,
	                           &WgForge::WireguardInterfaceController::updateConfig);


	// Starting HTTP server
	spd_logger->info(
	    std::format("Starting HTTP Server on {}:{}", SRV_CFG.addr, SRV_CFG.listenPort));

	WgForge::WgForgeServer server(SRV_CFG.addr, std::to_string(SRV_CFG.listenPort));
	server.setHandler(std::move(request_handler));

	// Scheduler
	spd_logger->info("Configuring server scheduler");
	auto scheduler = std::make_shared<WgForge::PeerExpiryScheduler>(
	    server.getContext(), injector.create<std::shared_ptr<WgForge::PeerService>>(), spd_logger);
	scheduler->setUpdateTime(SRV_CFG.peer_expire_update);
	scheduler->start();

	spd_logger->info("Server successfully started and ready to accept connections!");
	server.run();
  }

  catch (const std::exception& e) {
	if (spd_logger)
	  spd_logger->critical(e.what());
	else
	  std::cerr << "[FATAL ERROR] " << e.what() << std::endl;
	return 1;
  }

  return 0;
}
