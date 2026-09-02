#include "handlers.h"
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/route/addr.h>
#include <libnl3/netlink/route/link.h>
#include <sqlite3.h>

namespace WgForge {

void interface__get(Request req, Response res, EndpointArgs &args) {
  if (req.method() != http::verb::get) {
    res.result(http::status::method_not_allowed);
    res.body() = "Only GET";
    res.prepare_payload();
    return;
  }

  auto &iface = args.iface;

  // Iface address
  nl_sock *sock = nl_socket_alloc();
  if (sock == nullptr) {
    res.result(http::status::internal_server_error);
    res.prepare_payload();
    return;
  }

  auto ec = nl_connect(sock, NETLINK_ROUTE);
  if (ec < 0) {
    nl_socket_free(sock);

    res.result(http::status::internal_server_error);
    res.prepare_payload();
    return;
  }

  nl_cache *link_cache;
  if ((ec = rtnl_link_alloc_cache(sock, AF_INET, &link_cache)) < 0) {
    nl_close(sock);
    nl_socket_free(sock);

    res.result(http::status::internal_server_error);
    res.prepare_payload();
    return;
  }

  rtnl_link *link = rtnl_link_get_by_name(link_cache, iface.getName());
  if (link == nullptr) {
    nl_cache_free(link_cache);
    nl_close(sock);
    nl_socket_free(sock);

    res.result(http::status::internal_server_error);
    res.prepare_payload();
    return;
  }

  nl_cache *addr_cache;
  if ((ec = rtnl_addr_alloc_cache(sock, &addr_cache)) < 0) {
    rtnl_link_put(link);
    nl_cache_free(link_cache);
    nl_close(sock);
    nl_socket_free(sock);

    res.result(http::status::internal_server_error);
    res.prepare_payload();
    return;
  }

  auto ifaceIndex = rtnl_link_get_ifindex(link);
  char ip_addr[INET_ADDRSTRLEN]{'\0'};

  nl_object *object = nl_cache_get_first(addr_cache);
  while (object) {
    rtnl_addr *addr = reinterpret_cast<rtnl_addr *>(object);
    if (rtnl_addr_get_ifindex(addr) == ifaceIndex) {
      nl_addr *local = rtnl_addr_get_local(addr);
      if (local) {
        nl_addr2str(local, ip_addr, sizeof(ip_addr));
        break;
      }
    }

    object = nl_cache_get_next(object);
  }

  nl_cache_free(addr_cache);
  rtnl_link_put(link);
  nl_cache_free(link_cache);
  nl_close(sock);
  nl_socket_free(sock);

  // Failed to found ip address
  if (ip_addr[0] == '\0') {
    res.result(http::status::internal_server_error);
    res.prepare_payload();
    return;
  }

  json::object obj;
  obj["interface_name"] = iface.getName() ? iface.getName() : "";
  obj["is_up"] = iface.isUp() ? true : false;
  obj["listen_port"] = iface.getPort();
  obj["ip_address"] = ip_addr;

  auto private_key_opt = iface.getPrivateKey();
  char buf[sizeof(wg_key_b64_string)]{};
  if (private_key_opt.has_value()) {
    WgPrivateKey key = std::move(private_key_opt.value());
    wg_key_to_base64(buf, key.data());
    obj["private_key"] = buf;
  }

  auto public_key_opt = iface.getPublicKey();
  if (public_key_opt.has_value()) {
    WgPublicKey key = std::move(public_key_opt.value());
    wg_key_to_base64(buf, key.data());
    obj["public_key"] = buf;
  }

  res.result(http::status::ok);
  res.set(http::field::content_type, "application/json");
  res.body() = json::serialize(obj);
  res.prepare_payload();
}

void interface__patch(Request req, Response res, EndpointArgs &args) {
  if (req.method() != http::verb::patch) {
    res.result(http::status::method_not_allowed);
    res.body() = "ONLY PATCH";
    res.prepare_payload();
    return;
  }

  auto &iface = args.iface;
  // TODO
  // Update iface options and write it into config
}

void peers(Request req, Response res, EndpointArgs &args) {
  if (req.method() != http::verb::get) {
    res.result(http::status::method_not_allowed);
    res.body() = "Only GET";
    res.prepare_payload();
    return;
  }

  json::object j;
  auto *pdb = args.pdb;
  sqlite3_stmt *stmt = nullptr;
  constexpr const char query[] =
      R"(
      SELECT 
            id, 
            Public_key,
            Preshared_key,
            Private_key,
            Persistent_keepalive, 
            Description, 
            Created_at, 
            Updated_at 
        FROM Peers 
        WHERE Deleted_at IS NOT NULL
      )";
  if (sqlite3_prepare_v2(pdb, query, -1, &stmt, nullptr) != SQLITE_OK) {
    constexpr auto error = "Failed to get peers data. Error message: ";
    log_error("/peers [GET]: ", error, sqlite3_errmsg(pdb));

    res.result(http::status::internal_server_error);
    res.set(http::field::content_type, "application/json");
    j["error"] = std::string(error) + sqlite3_errmsg(pdb);
    res.body() = json::serialize(j);
    res.prepare_payload();

    return;
  }

  json::object o;
  json::array arr;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    o["id"] = sqlite3_column_int64(stmt, 0);
    o["public_key"] =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    o["private_key"] =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    o["persistent_keepalive"] = sqlite3_column_int(stmt, 3);
    o["description"] =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
    o["created_at"] =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
    o["updated_at"] =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));

    arr.push_back(o);
  }

  j["peers"] = arr;

  res.result(http::status::ok);
  res.set(http::field::content_type, "application/json");
  res.body() = json::serialize(j);
  res.prepare_payload();
}

void peer__get(Request req, Response res, EndpointArgs &args) {}

void peer__post(Request req, Response res, EndpointArgs &args) {
  if (req.method() != http::verb::post) {
    res.result(http::status::method_not_allowed);
    res.body() = "Only POST";
    res.prepare_payload();
    return;
  }

  try {
    auto j = json::parse(req.body()).as_object();
    std::string public_key_b64 = j["public_key"].as_string().c_str();

    // Get public key
    WgKey::key_type public_key_data{};
    if (wg_key_from_base64(public_key_data.data(), public_key_b64.c_str()))
      throw std::runtime_error("Invalid base64 public key");

    // Get preshared key
    bool has_preshared_key{false};
    WgKey::key_type preshared_key_data{};
    if (auto r = j["preshared_key"].try_as_string()) {
      has_preshared_key = true;
      std::string preshared_key_b64 = r->c_str();
      if (wg_key_from_base64(preshared_key_data.data(),
                             preshared_key_b64.c_str()))
        throw std::runtime_error("Invalid base64 preshared key");
    }

    // Get allowedip
    WgAllowedIP allowedip{j["allowed_ip"].as_string().c_str()};

    // Get persistent keepalive
    uint16_t persistent_keepalive{};
    if (auto r = j["persistent_keepalive"].try_to_number<uint16_t>())
      persistent_keepalive = *r;

    json::object response;

    // Generate objects and insert into peer
    WgPublicKey pbl_key{public_key_data};
    if (args.iface.hasPeerWithPublicKey(pbl_key)) {
      response["error"] =
          "Peer with given public key exists: \"" + public_key_b64 + "\"";
      res.result(http::status::bad_request);
    } else {
      WgPeer peer{&pbl_key};
      peer.setPersistentKeepAlive(persistent_keepalive);
      peer.addAllowedIP(std::move(allowedip));
      if (has_preshared_key) {
        WgPresharedKey prs_key{preshared_key_data};
        peer.setPresharedKey(std::move(prs_key));
      }

      args.iface.addPeer(std::move(peer));

      response["status"] = "created";
      res.result(http::status::created);
    }

    res.body() = json::serialize(response);
    res.prepare_payload();
  } catch (const std::exception &e) {
    json::object response;
    response["error"] = e.what();

    res.result(http::status::bad_request);
    res.body() = json::serialize(response);
    res.prepare_payload();
  }
}

void peer__delete(Request req, Response res, EndpointArgs &args) {
  if (req.method() != http::verb::delete_) {
    res.result(http::status::method_not_allowed);
    res.body() = "Only DELETE";
    res.prepare_payload();
    return;
  }

  try {
    auto j = json::parse(req.body()).as_object();
    std::string public_key = j["public_key"].as_string().c_str();

    WgKey::key_type public_key_data{};
    if (wg_key_from_base64(public_key_data.data(), public_key.data()))
      throw std::runtime_error("Invalide base64 public key");

    args.iface.removePeer(WgPublicKey{public_key_data});

    json::object response;
    response["status"] = "deleted";

    res.result(http::status::ok);
    res.body() = json::serialize(response);
    res.prepare_payload();
  } catch (const std::exception &e) {
    json::object response;
    response["error"] = e.what();

    res.result(http::status::bad_request);
    res.body() = json::serialize(response);
    res.prepare_payload();
  }
}

void peer__patch(Request req, Response res, EndpointArgs &args) {}

void peer_config(Request req, Response res, EndpointArgs &args) {}

} // namespace WgForge
