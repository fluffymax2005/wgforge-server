#include "initializer.h"
#include "handlers.h"
#include "inetutils.h"
#include "subnetscanner.h"

#include <arpa/inet.h>
#include <cerrno>
#include <format>
#include <libeconf.h>

#include <sqlite3.h>

namespace WgForge {

sqlite3 *setup_database(const char *iface_name) {
  // SQlite db filepath
  constexpr const char prefix[] = "/var/lib/wgforge/";
  constexpr const char ext[] = ".sqlite";
  constexpr size_t BUF_SIZE =
      (sizeof(prefix) - 1) + (IFNAMSIZ - 1) + sizeof(ext);
  char buf[BUF_SIZE]{};

#ifndef __PHONY_CONFIG__
  std::snprintf(buf, BUF_SIZE, "%s%s%s", prefix, iface_name, ext);
#else
  std::snprintf(buf, BUF_SIZE, "./%s%s", iface_name, ext);
#endif

  // Open SQlite db connection
  sqlite3 *db = nullptr;
  int rc = sqlite3_open_v2(
      buf, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);

  if (rc != SQLITE_OK) {
    // Get Linux error code
    rc = sqlite3_extended_errcode(db) >> 8;

    // Access denied or directory provided
    if (rc == EACCES || rc == EISDIR) {
      sqlite3_close_v2(db);
      throw std::runtime_error(
          std::format("Can't open database by filepath \"{}\": {}", buf,
                      sqlite3_errmsg(db)));
    }

    // File does not exists. Open and initialize it
    db = __init_database(buf);
  }

  WgForge::log_info(db);

  // DB opened successfully
  return db;
}

sqlite3 *__init_database(const char *filepath) {
  sqlite3 *db = nullptr;
  int rc = sqlite3_open_v2(filepath, &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                               SQLITE_OPEN_FULLMUTEX,
                           nullptr);
  if (rc != SQLITE_OK) {
    rc = sqlite3_extended_errcode(db);
    throw std::runtime_error(
        std::format("Can't create database by filepath \"{}\": {}. SQLite "
                    "error code: {}. Linux error code: ",
                    filepath, sqlite3_errmsg(db), rc, rc >> 8));
  }

  // SQL queries
  constexpr const char CREATE_PEERS[] = R"(
    CREATE TABLE IF NOT EXISTS Peers (
        Id INTEGER PRIMARY KEY AUTOINCREMENT,
        Public_key TEXT UNIQUE NOT NULL CHECK(length(Public_key) = 44),
        Preshared_key TEXT CHECK(length(Preshared_key) = 44),
        Private_key TEXT CHECK(length(Private_key) = 44),
        Persistent_keepalive INTEGER CHECK(Persistent_keepalive BETWEEN 1 AND 65535),
        Description VARCHAR(25),
        Created_at DATETIME DEFAULT CURRENT_STAMP,
        Updated_at DATETIME,
        Deleted_at DATETIME
    );
  )";

  constexpr const char CREATE_ALLOWEDIPS[] = R"(
    CREATE TABLE IF NOT EXISTS AllowedIPs (
        Id INTEGER PRIMARY KEY AUTOINCREMENT,
        Peer_id INTEGER NOT NULL,
        CIDR TEXT NOT NULL,
        Created_at DATETIME DEFAULT CURRENT_STAMP,
        Deleted_at DATETIME,
        FOREIGN KEY (Peer_id) REFERENCES Peers(Id) ON DELETE CASCADE
    );
  )";

  // Making requests
  char *err_msg;
  if (sqlite3_exec(db, CREATE_PEERS, nullptr, nullptr, &err_msg) != SQLITE_OK) {
    std::string error = err_msg;
    sqlite3_free(err_msg);
    sqlite3_close_v2(db);
    throw std::runtime_error(
        std::format("Failed to create Peers table: {}", error));
  }

  if (sqlite3_exec(db, CREATE_ALLOWEDIPS, nullptr, nullptr, &err_msg) !=
      SQLITE_OK) {
    std::string error = err_msg;
    sqlite3_free(err_msg);
    sqlite3_close_v2(db);
    throw std::runtime_error(
        std::format("Failed to create AllowedIPs table: {}", error));
  }

  return db;
}

void setup_endpoints(EndpointArgs &args, RequestHandler &handler) {
  handler.add_route(http::verb::get, "/interface",
                    [&args](const auto &req, auto &res) {
                      WgForge::interface__get(req, res, args);
                    });

  handler.add_route(http::verb::patch, "/interface",
                    [&args](const auto &req, auto &res) {
                      WgForge::interface__patch(req, res, args);
                    });

  handler.add_route(
      http::verb::get, "/peers",
      [&args](const auto &req, auto &res) { WgForge::peers(req, res, args); });

  handler.add_route(http::verb::get, "/peer",
                    [&args](const auto &req, auto &res) {
                      WgForge::peer__get(req, res, args);
                    });

  handler.add_route(http::verb::post, "/peer",
                    [&args](const auto &req, auto &res) {
                      WgForge::peer__post(req, res, args);
                    });

  handler.add_route(http::verb::patch, "/peer",
                    [&args](const auto &req, auto &res) {
                      WgForge::peer__patch(req, res, args);
                    });

  handler.add_route(http::verb::delete_, "/peer",
                    [&args](const auto &req, auto &res) {
                      WgForge::peer__delete(req, res, args);
                    });

  handler.add_route(http::verb::get, "/peer/config",
                    [&args](const auto &req, auto &res) {
                      WgForge::peer_config(req, res, args);
                    });
}

econf_file *load_config(const char name[]) {
  // Full config filepath
  constexpr const char path[] = "/etc/wgforge/";
  constexpr const char ext[] = ".conf";
  char filepath[(sizeof(path) - 1) + (IFNAMSIZ - 1) + sizeof(ext)]{};
  std::snprintf(filepath, sizeof(filepath), "%s%s%s", path, name, ext);

  econf_file *config = nullptr;
  econf_err ec = econf_readFile(&config, filepath, "=", "#");
  if (ec != ECONF_SUCCESS) {
    if (ec != ECONF_NOFILE)
      __handle_libeconf_write_error(ec,
                                    "Error occured while reading config file");
    config = create_config(name);
  }

  return config;
}

WgForgeServer setup_wgforgeserver(econf_file *config) {
  auto srvCfg = get_server_config(config);

  if (!InetUtils::isPortFree(srvCfg.addr, srvCfg.listenPort))
    throw std::invalid_argument(
        "[Interface] provided config for IP address and port which are busy");

  return WgForgeServer(srvCfg.addr, std::to_string(srvCfg.listenPort));
}

WgInterface setup_wginterface(econf_file *config, const char name[]) {

  WgInterface iface(name);
  auto ifaceCfg = get_iface_config(config, name);

  if (ifaceCfg.checkSubnet && !SubnetScanner::isSubnetFree(ifaceCfg.addr))
    throw std::invalid_argument(
        std::format("Provided subnet \"{}\" contains host", ifaceCfg.addr));

  iface.setListenPort(ifaceCfg.listenPort);
  iface.setPrivateKey(std::move(ifaceCfg.private_key));
  iface.set();

  InetUtils::assignIpAddress(ifaceCfg);

  iface.bringUp();
  return iface;
}

econf_file *create_config(const char name[]) {

  // Initialize new config
  econf_file *config;
  constexpr const char errMsg[] = "Failed to create config";

  econf_err ec = econf_newKeyFile(&config, '=', '#');
  char buf[256];
  if (ec != ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // [Interface]
  // ListenPort=51820
  if ((ec = econf_setIntValue(config, "Interface", "ListenPort", 51820)) !=
      ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // [Interface]
  // PrivateKey=<private_key>
  wg_key_to_base64(buf, WgPrivateKey{}.data());
  if ((ec = econf_setStringValue(config, "Interface", "PrivateKey", buf)) !=
      ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // [Interface]
  // Address=<cidr>
  // Default suffix = 24
  const auto iface_address = SubnetScanner::findFreeSubnet(24);
  if ((ec = econf_setStringValue(config, "Interface", "Address",
                                 iface_address.c_str())) != ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // [Interface]
  // CheckSubnet=<true_or_false>
  // Should programm check subnet for WgInterface before start?
  // Default is true
  constexpr const char checkSubnet[] = "true";
  if ((ec = econf_setBoolValue(config, "Interface", "CheckSubnet",
                               checkSubnet)) != ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // [Server]
  // Address=<local_listen_address>
  const auto srv_address = InetUtils::findFreeLoopbackAddress(51819);
  if ((ec = econf_setStringValue(config, "Server", "Address",
                                 srv_address.c_str())) != ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // [Server]
  // ListenPort=51819
  if ((ec = econf_setIntValue(config, "Server", "ListenPort", 51819)) !=
      ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  // Full config filepath
  constexpr const char path[] = "/etc/wgforge/";
  constexpr const char ext[] = ".conf";
  char filepath[(sizeof(path) - 1) + (IFNAMSIZ - 1) + sizeof(ext)]{};
  std::snprintf(filepath, sizeof(filepath), "%s%s%s", path, name, ext);

  // Write file
  std::string_view path_view(filepath);
  const auto iface_start_index = path_view.find_last_of('/');
  path_view.copy(buf, iface_start_index);
  buf[iface_start_index] = '\0'; // directory path

  if ((ec = econf_writeFile(config, buf, filepath + iface_start_index + 1)) !=
      ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

  return config;
} // namespace WgForge

InterfaceConfig get_iface_config(econf_file *config, const char name[]) {
  InterfaceConfig ifaceCfg;

  // Extract iface name from config filepath
  std::strncpy(ifaceCfg.name, name, sizeof(ifaceCfg.name));

  // ListenPort
  int32_t port;
  econf_err ec = econf_getIntValue(config, "Interface", "ListenPort", &port);
  if (ec != ECONF_SUCCESS)
    __handle_libeconf_read_error(ec, "Interface", "ListenPort");
  if (port < 0 ||
      port > std::numeric_limits<decltype(InterfaceConfig::listenPort)>::max())
    throw std::runtime_error(
        "Invalid listen port number provided for Wireguard interface");
  ifaceCfg.listenPort = port;

  // Address
  char *pstr;
  if ((ec = econf_getStringValue(config, "Interface", "Address", &pstr)) !=
      ECONF_SUCCESS)
    __handle_libeconf_read_error(ec, "Interface", "Address");
  auto spstr = std::unique_ptr<char[]>(pstr);

  try {
    WgAllowedIP allowed_ip{spstr.get()};
    ifaceCfg.proto_type = allowed_ip.getProto();
  } catch (...) {
    throw std::invalid_argument(
        "Invalid CIDR for \"Address\" provided by [Interface]");
  }

  std::strcpy(ifaceCfg.addr, spstr.get());

  // Private key
  if ((ec = econf_getStringValue(config, "Interface", "PrivateKey", &pstr)) !=
      ECONF_SUCCESS)
    __handle_libeconf_read_error(ec, "Interface", "PrivateKey");
  spstr.reset(pstr);

  WgKey::key_type private_key_raw;
  wg_key_from_base64(private_key_raw.data(), spstr.get());
  if (errno == EINVAL)
    throw std::invalid_argument("Inappropriate private key supplied");

  ifaceCfg.private_key = WgPrivateKey{private_key_raw};

  // CheckSubnet
  bool checkSubnet;
  if ((ec = econf_getBoolValue(config, "Interface", "CheckSubnet",
                               &checkSubnet)) != ECONF_SUCCESS)
    __handle_libeconf_read_error(ec, "Interface", "CheckSubnet");
  ifaceCfg.checkSubnet = checkSubnet;

  return ifaceCfg;
}
ServerConfig get_server_config(econf_file *config) {
  ServerConfig srvCfg;

  char *pstr;
  econf_err ec = econf_getStringValue(config, "Server", "Address", &pstr);
  if (ec != ECONF_SUCCESS)
    __handle_libeconf_read_error(ec, "Server", "Address");

  std::unique_ptr<char[]> spstr(pstr);
  std::strncpy(srvCfg.addr, spstr.get(), sizeof(srvCfg.addr) - 1);

  int32_t port;
  if ((ec = econf_getIntValue(config, "Server", "ListenPort", &port)) !=
      ECONF_SUCCESS)
    __handle_libeconf_read_error(ec, "Server", "ListenPort");

  if (port < 0 ||
      port > std::numeric_limits<decltype(ServerConfig::listenPort)>::max())
    throw std::runtime_error(
        "Invalid listen port number provided for Wireguard interface");
  srvCfg.listenPort = port;

  return srvCfg;
}

void __handle_libeconf_write_error(econf_err ec, const char msg[]) {
  throw std::runtime_error(std::format("%s: %s. Error code (libeconf): %i", msg,
                                       econf_errString(ec),
                                       static_cast<int>(ec)));
}

void __handle_libeconf_read_error(econf_err ec, const char group[],
                                  const char option[]) {
  std::string err_msg;
  switch (ec) {
  case ECONF_NOKEY:
    err_msg = std::format(
        "Error config parsing, no \"{}\" option in [{}] found", option, group);
    break;
  case ECONF_NOGROUP:
    err_msg = std::format("Error config parsing, no [{}] found", group);
    break;
  case ECONF_MISSING_BRACKET:
    err_msg = std::format("Error config parsing, missing closing section "
                          "bracket for \"{}\" group",
                          group);
    break;
  case ECONF_MISSING_DELIMITER:
    err_msg = std::format(
        "Error config parsing, missing delimiter for \"{}\" in [{}]", option,
        group);
    break;
  case ECONF_EMPTY_SECTION_NAME:
    err_msg = "Error config parsing, empty section name found";
    break;
  case ECONF_TEXT_AFTER_SECTION:
    err_msg = "Error config parsing, text after section found";
    break;
  case ECONF_OPTION_NOT_FOUND:
    err_msg =
        std::format("Error config parsing, option \"{}\" not found", option);
    break;
  default:
    err_msg = std::format(
        "Error occures while reading file for option \"{}\" in [{}]", option,
        group);
  }

  throw std::runtime_error(std::format("{}: {}. Error code (libeconf): {}",
                                       err_msg, econf_errString(ec),
                                       static_cast<int>(ec)));
}

} // namespace WgForge
