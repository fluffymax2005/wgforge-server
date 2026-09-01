
#include "handlers.h"
#include "initializer.h"
#include "request_handler.h"
#include "wgforgeserver.h"

#include <exception>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <wginterface.hpp>

int main(int argc, char *argv[]) {
  // Check command line arguments.
  if (argc != 2) {
    std::cerr << "Usage: wgforgeserver <interface_name>\n";
    std::cerr << "  For example, try:\n";
    std::cerr << "    wgforgeserver wg0\n";
    return 1;
  }

  try {

    // Config
    econf_file *config = WgForge::load_config(argv[1]);

#if defined(_DEBUG) || !defined(NDEBUG)
    WgForge::log_info("Configuring Wireguard interface");
#endif

    // Wireguard iface
    WgForge::WgInterface iface = WgForge::setup_wginterface(config, argv[1]);

#if defined(_DEBUG) || !defined(NDEBUG)
    WgForge::log_info("Configuring SQLite Database");
#endif

    // SQLite3 DB
    sqlite3 *db = WgForge::setup_database(argv[1]);

#if defined(_NDEBUG) || !defined(NDEBUG)
    WgForge::log_info("Configuring HTTP server endpoints");
#endif

    WgForge::EndpointArgs args{iface, db};
    WgForge::RequestHandler handler;
    WgForge::setup_endpoints(args, handler);

#if defined(_NDEBUG) || !defined(NDEBUG)
    WgForge::log_info("Configuring HTTP server");
#endif

    // HTTP server
    WgForge::WgForgeServer s = WgForge::setup_wgforgeserver(config);
    s.setHandler(std::move(handler));

#if defined(_DEBUG) || !defined(NDEBUG)
    const auto srvCfg = WgForge::get_server_config(config);
    const auto ifaceCfg = WgForge::get_iface_config(config, argv[1]);

    WgForge::log_info("HTTP server is listening on ", srvCfg.addr, ':',
                      srvCfg.listenPort);
    WgForge::log_info(
        "Wireguard VPN tunnel is listening on 0.0.0.0:", ifaceCfg.listenPort,
        ". Proto: ", ifaceCfg.proto_type == Protocol::IPv4 ? "IPv4" : "IPv6",
        ". VPN tunnel address: ", ifaceCfg.addr);
#endif

    // Release config
    econf_freeFile(config);

    // Run the server until stopped.
    s.run();

  } catch (std::exception &e) {
    WgForge::log_error(e.what());
    return 1;
  }

  return 0;
}
