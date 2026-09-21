#ifndef __WGFORGE_EXECUTION_INITIALIZER_H__
#define __WGFORGE_EXECUTION_INITIALIZER_H__

#include <config_manager.h>
#include <libeconf.h>
#include <sqlite_db_manager.h>

#include "defines.h"
#include "request_handler.h"
#include "wgforgeserver.h"

// Write to config phony data
#define __PHONY_CONFIG__

namespace WgForge {

// SQLite setup
void setup_database(SQLiteDbManager& db);

// WgForge::WgForgeServer setup
void setup_endpoints(EndpointArgs& args, RequestHandler& handler);

// WgInterface setup
WgInterface setup_wginterface(const ConfigManager& config, const char name[]);

// WgForge::WgForgeServer setup
WgForgeServer setup_wgforgeserver(const ConfigManager& config);

InterfaceConfig get_iface_config(const ConfigManager& config, const char name[]);
ServerConfig get_server_config(const ConfigManager& config);
WireguardConfig get_wireguard_config(const ConfigManager& config);

// Create config both for interface and http server. Implies lack of
// /usr/wgorge/<wg_interface_name>.conf
econf_file* create_config(std::string_view path, ConfigManager::ConfigReturnCode& rc) noexcept;

static std::optional<std::string> __get_public_ip();

} // namespace WgForge

#endif // __WGFORGE_EXECUTION_INITIALIZER_H__
