#ifndef __WGFORGE_DEFINES_H__
#define __WGFORGE_DEFINES_H__

#define __THREAD_SAFETY__ MultiThreaded

#define BLUE_COLOR "\033[34m"
#define RED_COLOR "\033[31m"
#define RESET_COLOR "\033[0m"

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <libwgcpp/wginterface.hpp>
#include <sqlite3.h>

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
  sqlite3 *pdb;

  EndpointArgs(WgInterface &iface, sqlite3 *db) : iface(iface), pdb(db) {}
};

struct InterfaceConfig {
  char name[IFNAMSIZ];
  in_port_t listenPort;
  char addr[46]; // [<ipv6_addr>]:<port> -> 1 + 39 + 1 + 1 + 3 + \0
  Protocol proto_type;
  WgPrivateKey private_key;
};

struct ServerConfig {
  char addr[40]; // IPv6 has maximum 39 chars
  in_port_t listenPort;
};

} // namespace WgForge

#endif // __WGFORGE_DEFINES_H__
