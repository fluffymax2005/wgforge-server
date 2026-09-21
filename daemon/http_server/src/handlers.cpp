#include "handlers.h"

#include <config_error_handler.h>
#include <inetutils.h>
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
	obj["ifaceName"] = iface.getName();
	obj["isUp"] = iface.isUp();
	obj["listenPort"] = iface.getPort();
	obj["ipAddress"] = ip_addr;
	obj["privateKey"] = iface.getPrivateKey()->toB64String();
	obj["publicKey"] = iface.getPublicKey()->toB64String();

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

	auto key_opt = get_optional_field<std::string_view>(j, "privateKey", false);
	if (key_opt && !WgKey::validateB64StringKey(*key_opt))
	  throw EndpointInvalidArgument("Invalid private key");

	auto addr_opt = get_optional_field<std::string_view>(j, "address", false);
	if (addr_opt && !WgAllowedIP::validate(addr_opt->data()))
	  throw EndpointInvalidArgument("Invalid listen address");

	auto port_opt = get_optional_field<int64_t>(j, "listenPort", false);
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

	if (port_opt && (rc = config.write("Interface", "ListenPort", port_opt.value()))) {
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
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	auto public_key = key_it->value;
	if (!WgKey::validateB64StringKey(public_key)) {
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
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

void peer__post(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::post) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only POST";
	res.prepare_payload();
	return;
  }

  auto& iface = args.iface;
  auto& db = args.db;
  auto& cfg = args.config;
  PeerConfig config;

  try {
	auto j = json::parse(req.body()).as_object();

	WgPrivateKey private_key;
	WgPublicKey public_key(private_key);
	do {
	  private_key.generate();
	  public_key = WgPublicKey(private_key);
	} while (iface.hasPeerWithPublicKey(public_key));

	std::string public_key_b64 = public_key.toB64String();

	config.record.public_key = public_key_b64;

	// Get whether preshared key should be generated
	auto preshared_key_opt = get_optional_field<bool>(j, "generatePresharedKey", false);
	WgPresharedKey preshared_key;
	if (preshared_key_opt && *preshared_key_opt) {
	  config.record.preshared_key = preshared_key.toB64String();
	}

	// Get allowedip
	auto allowed_ips_opt = get_optional_field<json::array>(j, "allowedIps");
	if (!allowed_ips_opt || allowed_ips_opt->size() == 0)
	  throw EndpointInvalidArgument("\"allowedIps\" missing");

	std::vector<WgAllowedIP> allowed_ips;
	allowed_ips.reserve(allowed_ips_opt->size());

	for (auto& ip : *allowed_ips_opt) {
	  try {
		auto& str = ip.as_string();
		allowed_ips.emplace_back(WgAllowedIP(str.c_str()));
		config.allowed_ips.emplace_back(std::move(str));
	  } catch (const boost::system::system_error&) {
		throw EndpointInvalidArgument("Invalid \"allowedIps\"");
	  } catch (const std::invalid_argument&) {
		throw EndpointInvalidArgument("Invalid \"allowedIps\"");
	  }
	}

	// Get CIDRs
	ConfigManager::ConfigReturnCode cfg_rc;
	const auto ifaceCIDR = cfg.tryRead<std::string>("Interface", "Address", &cfg_rc);
	if (cfg_rc)
	  throw EndpointRuntimeError(ConfigErrorHandler::toString(cfg_rc));

	SQLiteDbManager::SQLiteReturnCode db_rc;
	auto cidrs = db.getAllPeersAddresses(&db_rc);
	if (db_rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(db_rc));

	const auto cidr = InetUtils::findFreeClientTunnelCIDR(
	    ifaceCIDR->c_str(), std::vector<std::string_view>(cidrs.begin(), cidrs.end()));
	if (cidr.length() == 0)
	  throw EndpointInvalidArgument("Out of interface capacity");
	config.record.cidr = std::move(cidr);

	// Get persistent keepalive
	auto persistent_keepalive_opt = get_optional_field<int64_t>(j, "persistentKeepalive", false);
	if (!persistent_keepalive_opt) {
	  config.record.persistent_keepalive = 25;
	} else {
	  if (*persistent_keepalive_opt > std::numeric_limits<uint16_t>::max() ||
	      *persistent_keepalive_opt <= 0)
		throw EndpointInvalidArgument("\"persistentKeepalive\" must be in range [1; 65535]");
	  config.record.persistent_keepalive = *persistent_keepalive_opt;
	}

	// Get description
	auto description_opt = get_optional_field<std::string_view>(j, "description", false);
	if (description_opt)
	  config.record.description = std::move(*description_opt);

	auto rc = db.addPeer(config);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	// Generate objects and insert into peer
	WgPeer peer(&public_key);
	peer.setPersistentKeepAlive(*persistent_keepalive_opt);

	for (auto& ip : allowed_ips)
	  peer.addAllowedIP(std::move(ip));

	if (preshared_key_opt)
	  peer.setPresharedKey(std::move(preshared_key));

	iface.addPeer(std::move(peer));

	json::object o;
	o["publicKey"] = std::move(public_key_b64);
	if (preshared_key_opt)
	  o["presharedKey"] = *config.record.preshared_key;
	o["cidr"] = cidr;

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

void peer_rotate(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only PATCH";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;

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
	auto old_key_it = params.find("oldPublicKey");
	if (old_key_it == params.end())
	  throw EndpointInvalidArgument("\"oldPublicKey\" not provided");
	if (!WgKey::validateB64StringKey(old_key_it->value.data()))
	  throw EndpointInvalidArgument("\"oldPublicKey\" is invalid");

	auto new_key_it = params.find("newPublickey");
	if (new_key_it == params.end())
	  throw EndpointInvalidArgument("\"newPublicKey\" not provided");
	if (!WgKey::validateB64StringKey(new_key_it->value.data()))
	  throw EndpointInvalidArgument("\"newPublicKey\" is invalid");


	const std::string_view old_key_sv = old_key_it->value;
	const std::string_view new_key_sv = new_key_it->value;
	if (old_key_sv == new_key_sv)
	  throw EndpointInvalidArgument("Old and new public key must differ from each other");

	auto rc = db.rotatePeerPublicKey(old_key_sv, new_key_sv);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	try {
	  iface.rotatePeersPublicKey(WgPublicKey(old_key_sv), WgPublicKey(new_key_sv));
	} catch (const WgException& e) {
	  if ((rc = db.rotatePeerPublicKey(new_key_sv, old_key_sv)))
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	  auto ec = e.getCode();
	  if (ec == ENOENT)
		throw EndpointInvalidArgument("Peer not found");
	  else
		throw EndpointRuntimeError(
		    std::format("Failed to rotate peer's public key \"{}\". Error occured while applying "
			            "changes to kernel",
			            old_key_sv));
	}



	res.result(http::status::no_content);
  } catch (const EndpointInvalidArgument& e) {
	json::object o;
	o["error"] = e.what();

	res.result(http::status::bad_request);
	res.body() = json::serialize(o);
	res.set(http::field::content_type, "application/json");
  } catch (const EndpointRuntimeError& e) {
	log_error("/peer/rotate [PATCH]: ", e.what());

	res.result(http::status::internal_server_error);
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
	const auto& target = req.target();
	auto r = boost::urls::parse_uri(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	auto public_key = key_it->value;
	if (!WgKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");

	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key->data())))
	  throw EndpointInvalidArgument("Peer does not exist");

	auto rc = db.deletePeer(public_key->data());
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	iface.removePeer(WgPublicKey{public_key->data()});

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
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only PATCH";
	res.prepare_payload();
	return;
  }

  auto& iface = args.iface;
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
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	auto public_key = key_it->value;
	if (!WgKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");


	uint8_t changes = 0;
	json::object j = json::parse(req.body()).as_object();
	auto public_key_opt = get_optional_field<std::string_view>(j, "publicKey");
	const std::string_view public_key_sv = public_key_opt->data();
	if (!WgKey::validateB64StringKey(public_key_sv))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key_sv)))
	  throw EndpointInvalidArgument("Peer not found");

	PeerConfig origin_cfg;
	auto rc = db.getPeer(public_key_sv, origin_cfg);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));
	PeerConfig config = origin_cfg;


	auto preshared_key_opt = get_optional_field<std::string_view>(j, "presharedKey", false);
	if (preshared_key_opt) {
	  const std::string_view preshared_key_sv = preshared_key_opt->data();
	  if (!WgKey::validateB64StringKey(preshared_key_sv))
		throw EndpointInvalidArgument("Invalid \"presharedKey\"");
	  config.record.preshared_key = preshared_key_sv;
	  changes++;
	}

	auto persistent_keepalive_opt = get_optional_field<int64_t>(j, "persistentKeepalive", false);
	if (persistent_keepalive_opt) {
	  auto persistent_keepalive = *persistent_keepalive_opt;
	  if (persistent_keepalive <= 0 ||
	      persistent_keepalive >= std::numeric_limits<std::uint16_t>::max())
		throw EndpointInvalidArgument(
		    "Incorrect \"persistentKeepalive\". It must be in range [1; 65535]");
	  config.record.persistent_keepalive = persistent_keepalive;
	  changes++;
	}

	auto description_opt = get_optional_field<std::string_view>(j, "description", false);
	if (description_opt) {
	  config.record.description = *description_opt;
	  changes++;
	}

	// Extract "removeAllowedIps" and "addAllowedIps" functor
	auto getAllowedIpsVec = [&](std::string_view field_name) -> std::vector<std::string> {
	  auto allowedips_opt = get_optional_field<json::array>(j, field_name, false);
	  std::vector<std::string> allowedips_vec;
	  if (allowedips_opt) {
		auto allowedips = *allowedips_opt;
		for (const auto& ip : allowedips) {
		  if (!ip.is_string())
			throw EndpointInvalidArgument(std::format("Invalid \"{}\"", field_name));
		  std::string_view ip_sv = ip.as_string().data();
		  if (!WgAllowedIP::validate(ip_sv.data()))
			throw EndpointInvalidArgument(
			    std::format("Incorrect element is \"{}\": \"{}\"", field_name, ip_sv));
		  allowedips_vec.emplace_back(ip_sv);
		}
		changes++;
	  }

	  return allowedips_vec;
	};



	auto remove_allowedips_vec = getAllowedIpsVec("removeAllowedIps");
	auto add_allowedips_vec = getAllowedIpsVec("addAllowedIps");

	PeerRecord peer_record = std::move(config.record);
	if (changes) {
	  if ((rc = db.updatePeer(public_key_sv, peer_record)))
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	  peer_record = std::move(origin_cfg.record);
	  if (remove_allowedips_vec.size() &&
	      (rc = db.removeAllowedIPs(public_key_sv, remove_allowedips_vec))) {
		if ((rc = db.updatePeer(public_key_sv, peer_record)))
		  throw EndpointRuntimeError(
		      "Failed to remove AllowedIps then failed to recover peer config");
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));
	  }

	  if (add_allowedips_vec.size() && (rc = db.addAllowedIPs(public_key_sv, add_allowedips_vec))) {
		if ((rc = db.updatePeer(public_key_sv, peer_record)))
		  throw EndpointRuntimeError("Failed to add AllowedIps then failed to recover peer config");
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));
	  }
	}

	res.result(http::status::ok);
  } catch (const EndpointRuntimeError& e) {
	log_error("/peer [PATCH]: ", e.what());

	res.result(http::status::internal_server_error);
  } catch (const EndpointInvalidArgument& e) {
	json::object o;
	o["error"] = e.what();

	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const std::exception& e) {
	log_error("/peer [PATCH]: ", e.what());
	res.result(http::status::internal_server_error);
  }

  res.prepare_payload();
}

void peer_config(Request req, Response res, EndpointArgs& args) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;
  const auto& wireguard_cfg = args.wireguard_cfg;

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
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	auto public_key = key_it->value;
	if (!WgKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");

	std::string public_endpoint;
	auto addr_it = params.find("publicEndpoint");
	if (addr_it != params.end()) {
	  if (!WgEndpoint::validate(addr_it->value.data()))
		throw EndpointInvalidArgument("Invalid \"publicEndpoint\"");
	  public_endpoint = addr_it->value.data();
	} else {
	  public_endpoint = wireguard_cfg.public_endpoint;
	}

	PeerConfig peer;
	auto rc = db.getPeer(public_key, peer);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc));

	std::string config;
	config.reserve(1024);
	config += std::format(
	    "# Wireguard Client configuration\n"
	    "# Provided by WgForge\n\n"
	    "[Interface]\n"
	    "# Client Private key - keep this SECRET\n"
	    "PrivateKey=<PRIVATE_KEY>\n"
	    "# Client VPN tunnel IP address\n"
	    "Address={}\n"
	    "# DNS servers to resolve server domain name (if used)\n"
	    "DNS=1.1.1.1, 8.8.8.8, 1.0.0.1\n\n",
	    peer.record.cidr);

	config += std::format(
	    "[Peer]\n"
	    "# Server Public key\n"
	    "PublicKey={}\n"
	    "# Server Endpoint\n"
	    "Endpoint={}\n"
	    "# Keepalive time in seconds for NAT\n"
	    "PersistentKeepalive={}\n",
	    iface.getPublicKey()->toB64String(), public_endpoint, peer.record.persistent_keepalive);

	if (peer.record.preshared_key)
	  config += std::format(
	      "# Preshared key\n"
	      "PresharedKey={}",
	      peer.record.preshared_key->c_str());

	for (auto i = 0; i < peer.allowed_ips.size(); i++) {
	  if (i == peer.allowed_ips.size() - 1)
		config += peer.allowed_ips[i];
	  else
		config += peer.allowed_ips[i] + ", ";
	}


	json::object j;
	j["config"] = std::move(config);
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
