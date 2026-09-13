#ifndef __WGFORGE_DEFINES_H__
#define __WGFORGE_DEFINES_H__

#define __THREAD_SAFETY__ MultiThreaded

#define BLUE_COLOR "\033[34m"
#define RED_COLOR "\033[31m"
#define RESET_COLOR "\033[0m"

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <config_manager.h>
#include <libeconf.h>
#include <libwgcpp/wginterface.hpp>
#include <sqlite3.h>
#include <sqlite_db_manager.h>

namespace WgForge {

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace json = boost::json;

using Request = const http::request<http::string_body> &;
using Response = http::response<http::string_body> &;

using WgInterface = WgInterface<__THREAD_SAFETY__>;
using WgPeer = WgPeer<__THREAD_SAFETY__>;
using WgKey = WgKey<__THREAD_SAFETY__>;
using WgPrivateKey = WgPrivateKey<__THREAD_SAFETY__>;
using WgPublicKey = WgPublicKey<__THREAD_SAFETY__>;
using WgPresharedKey = WgPresharedKey<__THREAD_SAFETY__>;
using WgAllowedIP = WgAllowedIP<__THREAD_SAFETY__>;
using WgEndpoint = WgEndpoint<__THREAD_SAFETY__>;

struct EndpointArgs {
  WgInterface &iface;
  ConfigManager &config;
  SQLiteDbManager &db;

  EndpointArgs(WgInterface &iface, ConfigManager &cfg, SQLiteDbManager &db)
      : iface(iface), config(cfg), db(db) {}
};

struct InterfaceConfig {
  char addr[INET6_ADDRSTRLEN +
            7]; // [<ipv6_addr>]:<port> -> 1 + 39 + 1 + 1 + 3 + \0
  WgPrivateKey private_key;
  char name[IFNAMSIZ];
  in_port_t listenPort;
  Protocol proto_type;
  bool checkSubnet;
};

struct ServerConfig {
  char addr[INET6_ADDRSTRLEN]; // IPv6 has maximum 39 chars
  in_port_t listenPort;
};

} // namespace WgForge

#endif // __WGFORGE_DEFINES_H__
