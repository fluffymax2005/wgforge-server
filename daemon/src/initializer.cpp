#include "initializer.h"
#include "handlers.h"
#include <cerrno>

#include <arpa/inet.h>
#include <format>
#include <libeconf.h>
#include <netlink/netlink.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <sqlite3.h>

namespace WgForge {

sqlite3 *setup_database(const char *iface_name) {
  // SQlite db filepath
  constexpr const char prefix[] = "/var/lib/wgforge/";
  constexpr const char ext[] = ".sqlite";
  constexpr size_t BUF_SIZE =
      (sizeof(prefix) - 1) + (IFNAMSIZ - 1) + sizeof(ext);
  char buf[BUF_SIZE]{};
  std::snprintf(buf, BUF_SIZE, "%s%s%s", prefix, iface_name, ext);

  // Open SQlite db connection
  sqlite3 *db = nullptr;
  int rc = sqlite3_open_v2(
      buf, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
  // Failed to open sqlite db
  if (rc != SQLITE_OK) {
    // Get Linux error code
    rc = sqlite3_extended_errcode(db) >> 8;

    // Access denied or directory provided
    if (rc == EACCES || rc == EISDIR) {
      sqlite3_close_v2(db);
      throw std::runtime_error(
          std::string("Can't open database by filepath \"") + buf +
          "\": " + sqlite3_errmsg(db));
    }

    // File does not exists. Open and initialize it
    return WgForge::__init_database(iface_name);
  }

  // DB opened successfully
  return db;
}

sqlite3 *__init_database(const char *filename) {
  sqlite3 *db = nullptr;
  int rc = sqlite3_open_v2(filename, &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                               SQLITE_OPEN_FULLMUTEX,
                           nullptr);
  if (rc != SQLITE_OK) {
    rc = sqlite3_extended_errcode(db);
    throw std::runtime_error(
        std::string("Can't create database by filepath") + filename + "\": " +
        sqlite3_errmsg(db) + ". Sqlite error code: " + std::to_string(rc) +
        ". Linux error code: " + std::to_string((rc >> 8)));
  }

  // TO DO: make db architecture + make sql queries for it

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

  return WgForgeServer(srvCfg.addr, std::to_string(srvCfg.listenPort));
}

WgInterface setup_wginterface(econf_file *config, const char name[]) {

  WgInterface iface(name);
  auto ifaceCfg = get_iface_config(config, name);

  iface.setListenPort(ifaceCfg.listenPort);
  iface.setPrivateKey(std::move(ifaceCfg.private_key));
  iface.set();

  __assign_ip_address(ifaceCfg);

  iface.bringUp();
  return iface;
}

void __assign_ip_address(const InterfaceConfig &config) {
  struct nl_sock *sock = nl_socket_alloc();
  if (sock == nullptr)
    throw std::runtime_error(
        "Failed to allocate nl socket to configure interface");

  auto ec = nl_connect(sock, NETLINK_ROUTE);
  if (ec < 0) {
    nl_socket_free(sock);
    throw std::runtime_error("Failed to connect nl socket to kernel");
  }

  struct nl_cache *cache;
  if ((ec = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to allocate rtnl link cache");
  }

  struct rtnl_link *link = rtnl_link_get_by_name(cache, config.name);
  if (link == nullptr) {
    nl_cache_free(cache);
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to get rtnl link");
  }

  struct rtnl_addr *addr = rtnl_addr_alloc();
  if (addr == nullptr) {
    rtnl_link_put(link);
    nl_cache_free(cache);
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to allocate rntl address");
  }

  rtnl_addr_set_ifindex(addr, rtnl_link_get_ifindex(link));
  rtnl_addr_set_family(addr, AF_INET);

  struct in_addr ipv4_addr;
  char
      ip_text_addr[sizeof(config.addr) - 5]{}; // not include [], :, port number
  const auto *slash_pos{std::strchr(config.addr, '/')};
  std::strncpy(ip_text_addr, config.addr,
               slash_pos - config.addr); // copy address
  inet_pton(AF_INET, ip_text_addr, &ipv4_addr);

  struct nl_addr *local_addr =
      nl_addr_build(AF_INET, &ipv4_addr, sizeof(ipv4_addr));
  if (local_addr == nullptr) {
    rtnl_addr_put(addr);
    rtnl_link_put(link);
    nl_cache_free(cache);
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to build network address");
  }

  rtnl_addr_set_local(addr, local_addr);

  int32_t prefix_len{};
  try {
    prefix_len = std::stoi(std::string(slash_pos + 1));
  } catch (...) {
    nl_addr_put(local_addr);
    rtnl_addr_put(addr);
    rtnl_link_put(link);
    nl_cache_free(cache);
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to convert prefix to int");
  }
  rtnl_addr_set_prefixlen(addr, prefix_len);

  ec = rtnl_addr_add(sock, addr, 0);

  nl_addr_put(local_addr);
  rtnl_addr_put(addr);
  rtnl_link_put(link);
  nl_cache_free(cache);
  nl_close(sock);
  nl_socket_free(sock);

  if (ec < 0)
    throw std::runtime_error("Failed to add interface address");
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
// TODO Implement finding free address space
#ifdef __PHONY_CONFIG__
  std::snprintf(buf, sizeof(buf), "10.0.0.1/24");
#else
#endif
  if ((ec = econf_setStringValue(config, "Interface", "Address", buf)) !=
      ECONF_SUCCESS)
    __handle_libeconf_write_error(ec, errMsg);

// [Server]
// Address=<local_listen_address>
// TODO Get loopback free address space where port 51819 (pre Wireguard
// default) is available. See up.
#ifdef __PHONY_CONFIG__
  std::snprintf(buf, sizeof(buf), "127.0.0.1");
#else
#endif
  if ((ec = econf_setStringValue(config, "Server", "Address", buf)) !=
      ECONF_SUCCESS)
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
                          "bracket for \"%s\" group",
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

  throw std::runtime_error(std::format("{}: {}. Error code (libeconf): %i",
                                       err_msg, econf_errString(ec),
                                       static_cast<int>(ec)));
}

} // namespace WgForge
