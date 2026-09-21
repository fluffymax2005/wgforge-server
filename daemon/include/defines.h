#ifndef __WGFORGE_DEFINES_H__
#define __WGFORGE_DEFINES_H__

#define BLUE_COLOR  "\033[34m"
#define RED_COLOR   "\033[31m"
#define RESET_COLOR "\033[0m"

#include <config_manager.h>
#include <libeconf.h>
#include <sqlite3.h>
#include <sqlite_db_manager.h>
#include <wgforge.h>

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <libwgcpp/wginterface.hpp>

namespace WgForge {

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace json = boost::json;

using Request = const http::request<http::string_body>&;
using Response = http::response<http::string_body>&;

struct EndpointArgs {
  WgInterface& iface;
  ConfigManager& config;
  SQLiteDbManager& db;
  const ServerConfig& server_cfg;
  const InterfaceConfig& iface_cfg;

  EndpointArgs(WgInterface& iface, ConfigManager& cfg, SQLiteDbManager& db,
               const ServerConfig& server_config, const InterfaceConfig& iface_config)
      : iface(iface), config(cfg), db(db), server_cfg(server_config), iface_cfg(iface_config) {}
};


}  // namespace WgForge

#endif  // __WGFORGE_DEFINES_H__
