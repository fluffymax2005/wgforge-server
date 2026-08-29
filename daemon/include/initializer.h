#ifndef __WGFORGE_EXECUTION_INITIALIZER_H__
#define __WGFORGE_EXECUTION_INITIALIZER_H__

#include "defines.h"
#include "request_handler.h"
#include "wgforgeserver.h"

#include <libeconf.h>

// Write to config phony data
#define __PHONY_CONFIG__

namespace WgForge {

// SQLite setup
sqlite3 *setup_database(const char *iface_name);
sqlite3 *__init_database(const char *filename);

// WgForge::WgForgeServer setup
void setup_endpoints(EndpointArgs &args, RequestHandler &handler);

// WgInterface setup
WgInterface setup_wginterface(econf_file *config, const char name[]);

// WgForge::WgForgeServer setup
WgForgeServer setup_wgforgeserver(econf_file *config);

// Load config
econf_file *load_config(const char name[]);

InterfaceConfig get_iface_config(econf_file *config, const char name[]);
ServerConfig get_server_config(econf_file *config);

// Insert error message of write operations via libeconf. Throw exception with
// human readable message
void __handle_libeconf_write_error(econf_err ec, const char msg[]);

// Insert error message of read operations via libeconf. Throw exception with
// human readable message
void __handle_libeconf_read_error(econf_err ec, const char group[],
                                  const char option[]);

// Assign IP address to Wireguard interface
void __assign_ip_address(const InterfaceConfig &config);

// Create config both for interface and http server. Implies lack of
// /usr/wgorge/<wg_interface_name>.conf
econf_file *create_config(const char name[]);
} // namespace WgForge

#endif // __WGFORGE_EXECUTION_INITIALIZER_H__
