#ifndef __WGFORGE_H__
#define __WGFORGE_H__

#include <arpa/inet.h>
#include <libeconf.h>
#include <sqlite3.h>

#include <wginterface.hpp>
#include <wgprivatekey.hpp>

#define __THREAD_SAFETY__ MultiThreaded

namespace WgForge {

// SQLiteDbManager

#define WGFORGE_DATABASE_OK                 SQLITE_OK
#define WGFORGE_INVALID_ARGUMENT            -1
#define WGFORGE_RUNTIME_ERROR               -2
#define WGFORGE_RECORD_EXIST                -3
#define WGFORGE_CONNECTION_CLOSED           -4
#define WGFORGE_PEER_EXIST                  -5
#define WGFORGE_PEER_NOT_EXIST              -6
#define WGFORGE_ALLOWEDIP_EXIST             -7
#define WGFORGE_ALLOWEDIP_NOT_EXIST         -8
#define WGFORGE_DATABASE_READONLY           -9
#define WGFORGE_DATABASE_STRATEGY_NO_TARGET -10

// ConfigManager

#define WGFORGE_CONFIG_OK                   ECONF_SUCCESS
#define WGFORGE_CONFIG_READONLY             -100
#define WGFORGE_CONFIG_CLOSED               -101
#define WGFORGE_CONFIG_STRATEGY_NO_TARGET   -102
#define WGFORGE_CONFIG_NOMEM                -103
#define WGFORGE_CONFIG_TRANSACTION_ACTIVE   -104
#define WGFORGE_CONFIG_TRANSACTION_INACTIVE -105
#define WGFORGE_CONFIG_RUNTIME_ERROR        -106

// ConfigWriteRegister

#define WGFORGE_CONFIG_WRITER_NOT_FOUND     -200

// ConfigReadRegister

#define WGFORGE_CONFIG_READER_NOT_FOUND     -300
#define WGFORGE_CONFIG_INVALID_TYPE         -301

// Typedefs

using WgInterface = WgInterface<__THREAD_SAFETY__>;
using WgPeer = WgPeer<__THREAD_SAFETY__>;
using WgKey = WgKey<__THREAD_SAFETY__>;
using WgPrivateKey = WgPrivateKey<__THREAD_SAFETY__>;
using WgPublicKey = WgPublicKey<__THREAD_SAFETY__>;
using WgPresharedKey = WgPresharedKey<__THREAD_SAFETY__>;
using WgAllowedIP = WgAllowedIP<__THREAD_SAFETY__>;
using WgEndpoint = WgEndpoint<__THREAD_SAFETY__>;

// Core structures

struct InterfaceConfig {
  char addr[INET6_ADDRSTRLEN];
  WgPrivateKey private_key;
  char name[IFNAMSIZ];
  in_port_t listenPort;
  Protocol proto_type;
  bool checkSubnet;
};

struct ServerConfig {
  char addr[INET6_ADDRSTRLEN];  // IPv6 has maximum 39 chars
  in_port_t listenPort;
};

}  // namespace WgForge

#endif
