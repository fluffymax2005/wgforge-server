#include "handlers.h"

#include <config_error_handler.h>
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/route/addr.h>
#include <libnl3/netlink/route/link.h>
#include <peer_config.h>
#include <sqlite3.h>
#include <sqlite_error_handler.h>

#include <boost/json/impl/serialize.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/url.hpp>
#include <sstream>
#include <wgkey.hpp>

#include "endpoint_invalid_argument.h"
#include "endpoint_runtime_error.h"
#include "endpoint_tools.h"

namespace WgForge {

void interface__get(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& iface = args.iface;

  try {
	    // Iface address
	nl_sock* sock = nl_socket_alloc();
	if (sock == nullptr) {
	  throw EndpointRuntimeError("Failed to alloc netlink socket");
	}

	auto ec = nl_connect(sock, NETLINK_ROUTE);
	if (ec < 0) {
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to connect netlink socket");
	}

	nl_cache* link_cache;
	if ((ec = rtnl_link_alloc_cache(sock, AF_INET, &link_cache)) < 0) {
	  nl_close(sock);
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to alloc rtnl link cache");
	}

	rtnl_link* link = rtnl_link_get_by_name(link_cache, iface.getName());
	if (link == nullptr) {
	  nl_cache_free(link_cache);
	  nl_close(sock);
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to get link by name");
	}

	nl_cache* addr_cache;
	if ((ec = rtnl_addr_alloc_cache(sock, &addr_cache)) < 0) {
	  rtnl_link_put(link);
	  nl_cache_free(link_cache);
	  nl_close(sock);
	  nl_socket_free(sock);

	  throw EndpointRuntimeError("Failed to alloc rtnl address cache");
	}

	auto ifaceIndex = rtnl_link_get_ifindex(link);
	char ip_addr[INET_ADDRSTRLEN]{'\0'};

	nl_object* object = nl_cache_get_first(addr_cache);
	while (object) {
	  rtnl_addr* addr = reinterpret_cast<rtnl_addr*>(object);
	  if (rtnl_addr_get_ifindex(addr) == ifaceIndex) {
		nl_addr* local = rtnl_addr_get_local(addr);
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
	  throw EndpointRuntimeError("Failed to find any IPv4 address corresponding to interface");
	}

	json::object obj;
	obj["interface_name"] = iface.getName() ? iface.getName() : "";
	obj["is_up"] = iface.isUp() ? true : false;
	obj["listen_port"] = iface.getPort();
	obj["ip_address"] = ip_addr;

	auto private_key_opt = iface.getPrivateKey();
	WgKey::b64_key_type buf{};
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
  } catch (const std::exception& e) {
	log_error("/interface [GET]: ", e.what());
	res.result(http::status::internal_server_error);
  }

  res.prepare_payload();
}

void interface__patch(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "ONLY PATCH";
	res.prepare_payload();
	return;
  }

  auto& config = args.config;

  try {
	auto j = json::parse(req.body()).as_object();

	auto key_opt = get_optional_field<std::string_view>(j, "Private_key", false);
	if (key_opt && !WgKey::validateB64StringKey(*key_opt))
	  throw EndpointInvalidArgument("Invalid private key");

	auto addr_opt = get_optional_field<std::string_view>(j, "Address", false);
	if (addr_opt && !WgAllowedIP::validate(addr_opt->data()))
	  throw EndpointInvalidArgument("Invalid listen address");

	auto port_opt = get_optional_field<uint64_t>(j, "ListenPort", false);
	if (port_opt && port_opt.value() > std::numeric_limits<in_port_t>::max())
	  throw EndpointInvalidArgument("Invalid listen port");

	ConfigManager::ConfigReturnCode rc = config.beginTransaction();
	if (rc)
	  throw EndpointRuntimeError("Failed to start transaction");

	if (key_opt && (rc = config.write("Interface", "PrivateKey", key_opt->data())))
	  throw EndpointRuntimeError(
	      ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"));

	if (addr_opt && (rc = config.write("Interface", "Address", addr_opt->data()))) {
	  config.rollBack();
	  throw EndpointRuntimeError(
	      ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"));
	}

	if (port_opt && config.write("Interface", "ListenPort", port_opt.value())) {
	  config.rollBack();
	  throw EndpointRuntimeError(
	      ConfigErrorHandler::writeToString(rc, "Failed to apply changes to config"));
	}

	config.commit();

	res.result(http::status::no_content);
  } catch (const boost::system::system_error&) {
	json::object j;
	j["error"] = "Invalid body. Check arguments types";

	res.result(http::status::bad_request);
	res.body() = json::serialize(j);

  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();

	res.result(http::status::bad_request);
	res.body() = json::serialize(j);
  } catch (const EndpointRuntimeError& e) {
	log_error("/interface [PATCH]: ", e.what());
	json::object j;
	j["error"] = "Failed to process request";

	res.result(http::status::internal_server_error);
	res.body() = json::serialize(j);
  } catch (const std::bad_alloc&) {
	log_error("Interface [PATCH]: No enough memory");
	json::object j;
	j["error"] = "Failed to process request";

	res.result(http::status::internal_server_error);
	res.body() = json::serialize(j);
  }

  res.set(http::field::content_type, "application/json");
  res.prepare_payload();
}

void peers(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;

  try {
	json::object j;
	json::array arr;

	SQLiteDbManager::SQLiteReturnCode rc;
	const auto peers = db.getAllPeers(&rc);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	for (auto& peer : peers) {
	  arr.emplace_back(std::move(peer));
	}

	j["peers"] = std::move(arr);

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const EndpointRuntimeError& e) {
	log_error("/peers [GET]: ", e.what());

	res.result(http::status::internal_server_error);
	res.set(http::field::content_type, "application/json");
  } catch (const std::exception& e) {
	log_error("/peers [GET]: ", e.what());
	res.result(http::status::internal_server_error);
  }

  res.prepare_payload();
}

void peer__get(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;

  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_uri(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto key_it = params.find("public_key");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"public_key\" not provided");

	auto public_key = key_it->value;
	if (!WgKey::validateB64StringKey(public_key)) {
	  throw EndpointInvalidArgument("Invalid \"public_key\"");
	}

	PeerConfig peer;
	SQLiteDbManager::SQLiteReturnCode rc = db.getPeer(public_key, peer);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	json::object o = std::move(peer);

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);

  } catch (const boost::system::system_error&) {
	json::object j;
	j["error"] = "Invalid body. Check arguments types";

	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);

  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();

	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const EndpointRuntimeError& e) {
	json::object j;
	j["error"] = e.what();

	res.result(http::status::internal_server_error);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::exception& e) {
	log_error("/peer [GET]: ", e.what());
	res.result(http::status::internal_server_error);
  }

  res.prepare_payload();
}

// TODO
// Implement getting tunnel IP address for client
void peer__post(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::post) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only POST";
	res.prepare_payload();
	return;
  }

  auto& iface = args.iface;
  auto& db = args.db;
  PeerConfig config;

  try {
	auto j = json::parse(req.body()).as_object();

	WgPrivateKey private_key;
	std::string private_key_b64;
	WgPublicKey public_key(private_key);
	std::string public_key_b64;
	do {
	  private_key.generate();
	  public_key = WgPublicKey(private_key);
	} while (iface.hasPeerWithPublicKey(public_key));

	private_key_b64 = private_key.toB64String();
	public_key_b64 = public_key.toB64String();

	config.record.private_key = std::move(private_key_b64);
	config.record.public_key = public_key_b64;

	    // Get whether preshared key should be generated
	auto preshared_key_opt = get_optional_field<bool>(j, "generate_preshared_key");
	WgPresharedKey preshared_key;
	std::string preshared_key_key_b64;
	if (preshared_key_opt && preshared_key_opt.value()) {
	  preshared_key_key_b64 = preshared_key.toB64String();
	  config.record.preshared_key = preshared_key_key_b64;
	}

	    // Get allowedip
	auto allowed_ips_opt = get_optional_field<json::array>(j, "allowed_ips");
	if (allowed_ips_opt && allowed_ips_opt->size() == 0)
	  throw EndpointInvalidArgument("\"allowed_ips\" missing");

	std::vector<WgAllowedIP> allowed_ip_vec;
	allowed_ip_vec.reserve(allowed_ips_opt->size());

	for (auto& ip : *allowed_ips_opt) {
	  try {
		auto& str = ip.as_string();
		allowed_ip_vec.emplace_back(WgAllowedIP(str.c_str()));
		config.allowed_ips.emplace_back(std::move(str));
	  } catch (const boost::system::system_error&) {
		throw EndpointInvalidArgument("Invalid \"Allowed_ips\"");
	  } catch (const std::invalid_argument&) {
		throw EndpointInvalidArgument("Invalid \"Allowed_ips\"");
	  }
	}

	    // Get persistent keepalive
	auto persistent_keepalive_opt = get_optional_field<uint64_t>(j, "persistent_keepalive");
	if (!persistent_keepalive_opt) {
	  config.record.persistent_keepalive = 25;
	} else {
	  if (persistent_keepalive_opt.value() > std::numeric_limits<uint16_t>::max())
		throw EndpointInvalidArgument("\"persistent_keepalive\" must be no greater than 65535");
	  config.record.persistent_keepalive = persistent_keepalive_opt.value();
	}

	    // Get description
	auto description_opt = get_optional_field<std::string_view>(j, "description", false);
	if (description_opt)
	  config.record.description = std::move(*description_opt);

	auto rc = db.addPeer(config);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	json::object response;

	    // Generate objects and insert into peer
	WgPeer peer(&public_key);
	peer.setPersistentKeepAlive(persistent_keepalive_opt.value());

	for (auto& allowed_ip : allowed_ip_vec)
	  peer.addAllowedIP(std::move(allowed_ip));

	if (preshared_key_opt && preshared_key_opt.value())
	  peer.setPresharedKey(std::move(preshared_key));

	iface.addPeer(std::move(peer));

	json::object o;
	o["status"] = "Peer created";
	o["public_key"] = std::move(public_key_b64);
	if (preshared_key_opt && preshared_key_opt.value())
	  o["preshared_key"] = std::move(preshared_key_key_b64);

	res.result(http::status::created);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();

	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::bad_alloc& e) {
	log_error("/peer: [POST]: ", e.what());
	res.result(http::status::internal_server_error);
  } catch (const std::exception& e) {
	log_error("/peer [POST]: ", e.what());
	json::object j;
	j["error"] = e.what();

	res.result(http::status::internal_server_error);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  }

  res.prepare_payload();
}

void peer__delete(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::delete_) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only DELETE";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;

  try {
	auto j = json::parse(req.body()).as_object();
	auto public_key_opt = get_optional_field<std::string_view>(j, "public_key");

	if (!WgKey::validateB64StringKey(public_key_opt->data()))
	  throw EndpointInvalidArgument("Invalid \"Public_key\"");

	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key_opt->data())))
	  throw EndpointInvalidArgument("Peer does not exist");

	iface.removePeer(WgPublicKey{public_key_opt->data()});

	res.result(http::status::no_content);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();

	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);

  } catch (const std::exception& e) {
	res.result(http::status::internal_server_error);
  }

  res.prepare_payload();
}

void peer__patch(Request req, Response res, EndpointArgs& args) {
}

// TODO
// Implement getting client tunnel addr
void peer_config(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;

  try {
	json::object j = json::parse(req.body()).as_object();
	auto public_key_opt = get_optional_field<std::string_view>(j, "public_key");
	if (!WgKey::validateB64StringKey(public_key_opt->data()))
	  throw EndpointInvalidArgument("Invalid \"public_key\"");

	auto address_opt = get_optional_field<std::string_view>(j, "address", false);
	char buf[sizeof(in_addr)];
	if (address_opt && inet_pton(AF_INET, address_opt->data(), buf) != 1)
	  throw EndpointInvalidArgument("Invalid \"address\"");

	PeerConfig peer;
	auto rc = db.getPeer(public_key_opt->data(), peer);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	std::ostringstream oss;
	oss << "[Interface]\n"
	    << "PrivateKey=" << peer.record.private_key->c_str() << '\n'
	    << "Address=" << /*Insert client tunnel ip address*/ '\n'
	    << "DNS=1.1.1.1, 8.8.8.8, 1.0.0.1\n"
	    << "[Peer]\n"
	    << "PublicKey=" << iface.getPublicKey()->toB64String() << '\n';

	if (peer.record.preshared_key)
	  oss << "PresharedKey=" << peer.record.preshared_key << '\n';
	oss << "Endpoint=" << (address_opt ? address_opt->data() : " ") << ":" << iface.getPort()
	    << '\n'
	    << "PersistentKeepalive=" << peer.record.persistent_keepalive << '\n'
	    << "AllowedIPs=";

	for (auto i = 0; i < peer.allowed_ips.size(); i++) {
	  if (i == peer.allowed_ips.size() - 1)
		oss << peer.allowed_ips[i];
	  else
		oss << peer.allowed_ips[i] << ", ";
	}

	j["config"] = oss.str();
	oss.str("");
	res.body() = json::serialize(j);
	res.set(http::field::content_type, "application/json");
	res.result(http::status::ok);
  } catch (const std::bad_alloc& e) {
	log_error("/config [GET]: ", e.what());
	json::object j;
	j["error"] = "Failed to proccess request";

	res.set(http::field::content_type, "application/json");
	res.result(http::status::internal_server_error);
	res.body() = json::serialize(j);
  } catch (const EndpointRuntimeError& e) {
	log_error("/config [GET]: ", e.what());
	json::object j;
	j["error"] = "Failed to proccess request";

	res.set(http::field::content_type, "application/json");
	res.result(http::status::internal_server_error);
	res.body() = json::serialize(j);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();

	res.set(http::field::content_type, "application/json");
	res.result(http::status::internal_server_error);
	res.body() = json::serialize(j);
  }
}

} // namespace WgForge
