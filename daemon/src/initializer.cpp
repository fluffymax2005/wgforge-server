#include "initializer.h"
#include "config_error_handler.h"
#include "config_read_write_register.h"
#include "config_write_functions.h"
#include "config_write_register.h"
#include "handlers.h"
#include "inetutils.h"
#include "sqlite_error_handler.h"
#include "subnetscanner.h"

#include <arpa/inet.h>
#include <cerrno>
#include <format>
#include <libeconf.h>

#include <sqlite3.h>

namespace WgForge {

void setup_database(SQLiteDbManager &db) {
  // SQL queries
  constexpr const char CREATE_PEERS[] = R"(
    CREATE TABLE IF NOT EXISTS Peers (
        Id INTEGER PRIMARY KEY AUTOINCREMENT,
        Public_key TEXT UNIQUE NOT NULL CHECK(length(Public_key) = 44),
        Preshared_key TEXT CHECK(length(Preshared_key) = 44),
        Private_key TEXT CHECK(length(Private_key) = 44),
        CIDR TEXT,
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
  SQLiteDbManager::SQLiteReturnCode rc = db.rawSql(CREATE_PEERS);
  if (rc)
    throw std::runtime_error(std::format("Failed to create Peers table. {}",
                                         SQLiteErrorHandler::toString(rc)));

  if ((rc = db.rawSql(CREATE_ALLOWEDIPS)))
    throw std::runtime_error(
        std::format("Failed to create AllowedIPs table. {}",
                    SQLiteErrorHandler::toString(rc)));
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

WgForgeServer setup_wgforgeserver(const ConfigManager &config) {
  auto srvCfg = get_server_config(config);

  if (!InetUtils::isPortFree(srvCfg.addr, srvCfg.listenPort))
    throw std::invalid_argument(
        "[Interface] provided config for IP address and port which are busy");

  return WgForgeServer(srvCfg.addr, std::to_string(srvCfg.listenPort));
}

WgInterface setup_wginterface(const ConfigManager &config, const char name[]) {

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

econf_file *create_config(std::string_view path,
                          ConfigManager::ConfigReturnCode &rc) noexcept {

  // Initialize new config
  econf_file *config;
  econf_err ec = econf_newKeyFile(&config, '=', '#');
  if (ec != ECONF_SUCCESS) {
    rc = static_cast<ConfigManager::ConfigReturnCode>(ec);
    return nullptr;
  }

  try {
    ConfigReadWriteRegister rw_reg;
    rw_reg.registerWriter<int32_t>(LibeconfWriters::writeInt);
    rw_reg.registerWriter<std::string>(LibeconfWriters::writeString);
    rw_reg.registerWriter<bool>(LibeconfWriters::writeBool);

    ConfigManager cfg(config, false);
    cfg.setReadWriteRegister(std::move(rw_reg));

    // [Interface]
    // ListenPort=51820
    if ((rc = cfg.write<int>("Interface", "ListenPort", 51820)))
      return nullptr;

    // [Interface]
    // PrivateKey=<private_key>
    char buf[128];
    wg_key_to_base64(buf, WgPrivateKey{}.data());
    if ((rc = cfg.write<std::string>("Interface", "PrivateKey", buf)))
      return nullptr;

    // [Interface]
    // Address=<cidr> Default suffix = 24
    const auto iface_address = SubnetScanner::findFreeSubnet(24);
    if ((rc = cfg.write<std::string>("Interface", "Address", iface_address)))
      return nullptr;

    // [Interface]
    // CheckSubnet=<true_or_false>
    // Should programm check subnet for WgInterface before start?
    // Default is true
    if ((rc = cfg.write<bool>("Interface", "CheckSubnet", true)))
      return nullptr;

    // [Server]
    // Address=<local_listen_address>
    const auto srv_address = InetUtils::findFreeLoopbackAddress(51819);
    if ((rc = cfg.write<std::string>("Server", "Address", srv_address)))
      return nullptr;

    // [Server]
    // ListenPort=51819
    if ((rc = cfg.write<int>("Server", "ListenPort", 51819)))
      return nullptr;

    // Write file
    const auto iface_start_index = path.find_last_of('/');
    path.copy(buf, iface_start_index);
    buf[iface_start_index] = '\0'; // directory path

    if ((ec = econf_writeFile(config, buf,
                              path.data() + iface_start_index + 1)) !=
        ECONF_SUCCESS)
      return nullptr;

    cfg.release();

  } catch (const std::bad_alloc &) {
    rc = WGFORGE_CONFIG_NOMEM;
    config = nullptr;
  }

  rc = ECONF_SUCCESS;
  return config;
} // namespace WgForge

InterfaceConfig get_iface_config(const ConfigManager &config,
                                 const char name[]) {
  InterfaceConfig ifaceCfg;

  // Extract iface name from config filepath
  std::strncpy(ifaceCfg.name, name, sizeof(ifaceCfg.name));

  // ListenPort
  ConfigReadRegister::ReadReturnCode rc;
  if (auto opt = config.tryRead<int32_t>("Interface", "ListenPort", &rc))
    ifaceCfg.listenPort = opt.value();
  else
    throw std::runtime_error(
        ConfigErrorHandler::readToString(rc, "Interface", "ListenPort"));

  if (ifaceCfg.listenPort < 0 ||
      ifaceCfg.listenPort >
          std::numeric_limits<decltype(InterfaceConfig::listenPort)>::max())
    throw std::runtime_error(
        "Invalid listen port number provided for Wireguard interface");

  // Address
  if (auto opt = config.tryRead<std::string>("Interface", "Address", &rc))
    std::strncpy(ifaceCfg.addr, opt->c_str(), sizeof(ifaceCfg.addr));
  else
    throw std::runtime_error(
        ConfigErrorHandler::readToString(rc, "Interface", "Address"));

  try {
    WgAllowedIP allowed_ip{ifaceCfg.addr};
    ifaceCfg.proto_type = allowed_ip.getProto();
  } catch (...) {
    throw std::invalid_argument(
        "Invalid CIDR for \"Address\" provided by [Interface]");
  }

  // Private key
  if (auto opt = config.tryRead<std::string>("Interface", "PrivateKey", &rc)) {
    if (!WgKey::validateB64StringKey(opt->c_str()))
      throw std::invalid_argument("Inappropriate private key supplied");
    ifaceCfg.private_key = WgPrivateKey(opt->c_str());
  } else
    throw std::runtime_error(
        ConfigErrorHandler::readToString(rc, "Interface", "PrivateKey"));

  // CheckSubnet
  if (auto opt = config.tryRead<bool>("Interface", "CheckSubnet", &rc))
    ifaceCfg.checkSubnet = *opt;

  return ifaceCfg;
}
ServerConfig get_server_config(const ConfigManager &config) {
  ServerConfig srvCfg;

  ConfigReadRegister::ReadReturnCode rc;
  if (auto opt = config.tryRead<std::string>("Server", "Address", &rc)) {
    char buf[sizeof(in_addr)];
    if (inet_pton(AF_INET, opt->c_str(), buf) != 1)
      throw std::invalid_argument(
          "Incorrect \"Address\" provided for [Server]");
    std::strcpy(srvCfg.addr, opt->c_str());
  } else {
    throw std::runtime_error(
        ConfigErrorHandler::readToString(rc, "Server", "Address"));
  }

  if (auto opt = config.tryRead<int32_t>("Server", "ListenPort", &rc))
    srvCfg.listenPort = opt.value();
  else
    throw std::runtime_error(
        ConfigErrorHandler::readToString(rc, "Server", "ListenPort"));

  if (srvCfg.listenPort < 0 ||
      srvCfg.listenPort >
          std::numeric_limits<decltype(ServerConfig::listenPort)>::max())
    throw std::runtime_error(
        "Invalid listen port number provided for Wireguard interface");

  return srvCfg;
}

} // namespace WgForge
