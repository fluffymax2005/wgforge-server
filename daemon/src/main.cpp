#include "config_error_handler.h"
#include "config_manager.h"
#include "handlers.h"
#include "initializer.h"
#include "request_handler.h"
#include "sqlite_db_manager.h"
#include "wgforgeserver.h"

#include <exception>
#include <format>
#include <iostream>
#include <sqlite3.h>
#include <wginterface.hpp>

// Create phony config for tests
#define __PHONY_CONFIG__

int main(int argc, char *argv[]) {
  // Check command line arguments.
  if (argc != 2) {
    std::cerr << "Usage: wgforgeserver <interface_name>\n";
    std::cerr << "  For example, try:\n";
    std::cerr << "    wgforgeserver wg0\n";
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

    WgForge::log_info("Configuring Wireguard interface");

    // Wireguard iface
    WgForge::WgInterface iface =
        WgForge::setup_wginterface(config_manager, argv[1]);

    WgForge::log_info("Configuring SQLite Database");

    // SQLite3 DB
    const std::string db_path(
        std::format("/var/lib/wgforge/{}.sqlite", argv[1]));
    WgForge::SQLiteDbManager db_manager(db_path, false);

    WgForge::log_info("Configuring HTTP server endpoints");

    WgForge::EndpointArgs args{iface, config_manager, db_manager};
    WgForge::RequestHandler handler;
    WgForge::setup_endpoints(args, handler);

    WgForge::log_info("Configuring HTTP server");

    // HTTP server
    WgForge::WgForgeServer s = WgForge::setup_wgforgeserver(config_manager);
    s.setHandler(std::move(handler));

    const auto srvCfg = WgForge::get_server_config(config_manager);
    const auto ifaceCfg = WgForge::get_iface_config(config_manager, argv[1]);

    WgForge::log_info("HTTP server is listening on ", srvCfg.addr, ':',
                      srvCfg.listenPort);
    WgForge::log_info(
        "Wireguard VPN tunnel is listening on 0.0.0.0:", ifaceCfg.listenPort,
        ". Proto: ", ifaceCfg.proto_type == Protocol::IPv4 ? "IPv4" : "IPv6",
        ". VPN tunnel address: ", ifaceCfg.addr);

    // Run the server until stopped.
    s.run();

  } catch (std::exception &e) {
    WgForge::log_error(e.what());
    return 1;
  }

  return 0;
}
