/*
 * peer_controller.cpp
 * wgforge-server — Backend to manage Wireguard
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#include "peer_controller.h"

namespace WgForge {

PeerController::PeerController(std::shared_ptr<IPeerService> peer_service,
                               std::shared_ptr<ILogger> logger) {
  if (peer_service == nullptr)
	throw std::invalid_argument("Peer service is nullptr");
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");
  peer_service_ = std::move(peer_service);
  logger_ = std::move(logger);
}

void PeerController::getPeer(Request req, Response res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;
  constexpr const char* URI = "/peer";
  constexpr const char* METHOD = "GET";
  std::exception_ptr ep;

  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
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

	std::string public_key = boost::urls::decode(key_it->value).value();
	if (!WgKey::validateB64StringKey(public_key)) {
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	}

	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key)))
	  throw EndpointNotFound("Peer not found");

	PeerConfig peer;
	SQLiteDbManager::SQLiteReturnCode rc = db.getPeer(public_key, peer);
	if (rc) {
	  if (rc == SQLITE_DONE)
		throw EndpointNotFound("Peer not found");
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);
	}

	json::object o = std::move(peer);

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

void PeerController::getAllPeers(Request req, Response res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  constexpr const char* URI = "/peers";
  constexpr const char* METHOD = "GET";
  std::exception_ptr ep;

  try {
	json::object j;
	json::array arr;

	SQLiteDbManager::SQLiteReturnCode rc;
	const auto peers = db.getAllPeers(&rc);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);

	for (auto& peer : peers) {
	  arr.emplace_back(std::move(peer));
	}

	j["peers"] = std::move(arr);

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, res);

  res.prepare_payload();
}

void PeerController::createPeer(Request req, Response res) {
  if (req.method() != http::verb::post) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only POST";
	res.prepare_payload();
	return;
  }

  auto handleFillPeerConfig = [](const json::object& j) -> PeerConfig {
	PeerConfig peer;
  };

  PeerConfig config;
  constexpr const char* URI = "/peer";
  constexpr const char* METHOD = "POST";
  std::exception_ptr ep;

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
	SQLiteDbManager::SQLiteReturnCode db_rc;
	auto cidrs = db.getAllPeersAddresses(&db_rc);
	if (db_rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(db_rc), URI, METHOD);

	const auto cidr = InetUtils::findFreeClientTunnelCIDR(iface_cfg.addr, cidrs);
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
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);

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
	o["privateKey"] = private_key.toB64String();
	if (preshared_key_opt)
	  o["presharedKey"] = *config.record.preshared_key;
	o["cidr"] = cidr;

	res.result(http::status::created);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, res);

  res.prepare_payload();
}

void PeerController::deletePeer(Request req, Response res) {
  if (req.method() != http::verb::delete_) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only DELETE";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;
  constexpr const char* URI = "/peer";
  constexpr const char* METHOD = "DELETE";
  std::exception_ptr ep;

  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
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

	std::string public_key = boost::urls::decode(key_it->value).value();
	if (!WgKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");

	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key.c_str())))
	  throw EndpointNotFound("Peer does not exist");

	auto rc = db.deletePeer(public_key.c_str());
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);

	iface.removePeer(WgPublicKey{public_key.c_str()});

	res.result(http::status::no_content);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

void PeerController::updatePeer(Request req, Response res) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only PATCH";
	res.prepare_payload();
	return;
  }

  auto& iface = args.iface;
  auto& db = args.db;
  constexpr const char* URI = "/peer";
  constexpr const char* METHOD = "PATCH";
  std::exception_ptr ep;

  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
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

	std::string public_key = boost::urls::decode(key_it->value).value();
	if (!WgKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key)))
	  throw EndpointNotFound("Peer not found");


	uint8_t changes = 0;
	json::object j = json::parse(req.body()).as_object();

	PeerConfig origin_cfg;
	auto rc = db.getPeer(public_key, origin_cfg);
	if (rc)
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);
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
	  if ((rc = db.updatePeer(public_key, peer_record)))
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);

	  peer_record = std::move(origin_cfg.record);
	  if (remove_allowedips_vec.size() &&
	      (rc = db.removeAllowedIPs(public_key, remove_allowedips_vec))) {
		if ((rc = db.updatePeer(public_key, peer_record)))
		  throw EndpointRuntimeError(
		      "Failed to remove AllowedIps then failed to recover peer config", URI, METHOD);
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);
	  }

	  if (add_allowedips_vec.size() && (rc = db.addAllowedIPs(public_key, add_allowedips_vec))) {
		if ((rc = db.updatePeer(public_key, peer_record)))
		  throw EndpointRuntimeError("Failed to add AllowedIps then failed to recover peer config",
		                             URI, METHOD);
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);
	  }
	}

	res.result(http::status::ok);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

void PeerController::rotatePeerKey(Request req, Response res) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only PATCH";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;
  constexpr const char* URI = "/peer/rotate";
  constexpr const char* METHOD = "PATCH";
  std::exception_ptr ep;

  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
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
	std::string old_key = boost::urls::decode(old_key_it->value).value();
	if (!WgKey::validateB64StringKey(old_key.c_str()))
	  throw EndpointInvalidArgument("\"oldPublicKey\" is invalid");

	if (!iface.hasPeerWithPublicKey(WgPublicKey(old_key)))
	  throw EndpointNotFound("Peer not found");

	WgPrivateKey private_key;
	WgPublicKey new_public_key(private_key);
	do {
	  private_key.generate();
	  new_public_key = WgPublicKey(private_key);
	} while (iface.hasPeerWithPublicKey(new_public_key));

	const auto new_key = new_public_key.toB64String();

	auto rc = db.rotatePeerPublicKey(old_key, new_key);
	if (rc) {
	  if (rc == WGFORGE_PEER_NOT_EXIST)
		throw EndpointInvalidArgument(SQLiteErrorHandler::toString(rc));
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);
	}

	try {
	  iface.rotatePeersPublicKey(WgPublicKey(old_key), WgPublicKey(new_key));
	} catch (const WgException& e) {
	  if ((rc = db.rotatePeerPublicKey(new_key, old_key)))
		throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);

	  auto ec = e.getCode();
	  if (ec == ENOENT)
		throw EndpointInvalidArgument("Peer not found");
	  else
		throw EndpointRuntimeError(
		    std::format("Failed to rotate peer's public key \"{}\". Error occured while applying "
			            "changes to kernel",
			            old_key),
		    URI, METHOD);
	}

	json::object o;
	o["publicKey"] = std::move(new_key);
	o["privateKey"] = private_key.toB64String();

	res.body() = json::serialize(o);
	res.set(http::field::content_type, "application/json");
	res.result(http::status::ok);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

void PeerController::getPeersConfig(Request req, Response res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  auto& db = args.db;
  auto& iface = args.iface;
  const auto& wireguard_cfg = args.wireguard_cfg;
  constexpr const char* URI = "/peer/config";
  constexpr const char* METHOD = "GET";
  std::exception_ptr ep;

  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
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

	std::string public_key = boost::urls::decode(key_it->value).value();
	if (!WgKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	if (!iface.hasPeerWithPublicKey(WgPublicKey(public_key)))
	  throw EndpointNotFound("Peer not found");


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
	  throw EndpointRuntimeError(SQLiteErrorHandler::toString(rc), URI, METHOD);

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
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleEndpointException(ep, res);

  res.prepare_payload();
}

void PeerController::handleException(std::exception_ptr ep, Response& res) {
  try {
	if (ep)
	  std::rethrow_exception(ep);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const EndpointRuntimeError& e) {
	log_error(e.getUri(), " [", e.getMethod(), "]: ", e.what());
	res.result(http::status::internal_server_error);
  } catch (const EndpointNotFound& e) {
	json::object o;
	o["error"] = e.what();
	res.result(http::status::not_found);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const boost::system::system_error& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::exception& e) {
	log_error("Unexpected error: ", e.what());
	res.result(http::status::internal_server_error);
	res.body() = "Internal server error";
  }
}

}  // namespace WgForge
