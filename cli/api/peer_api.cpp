/*
 * peer_api.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#include "peer_api.h"

#include <netinet/in.h>

#include <boost/beast.hpp>
#include <boost/json/conversion.hpp>
#include <boost/json/value_from.hpp>
#include <boost/url.hpp>
#include <format>
#include <stdexcept>

namespace WgForge {

namespace beast = boost::beast;
namespace http = beast::http;

PeerApi::PeerApi(std::shared_ptr<IHttpSender> sender, std::shared_ptr<ISerializer<Peer>> serializer,
                 std::shared_ptr<ConfigManager> cfg_mgr) {
  if (sender == nullptr)
	throw std::invalid_argument("Http sender is nullptr");
  if (serializer == nullptr)
	throw std::invalid_argument("Peer serializer is nullptr");
  if (cfg_mgr == nullptr)
	throw std::invalid_argument("Config manager is nullptr");
  sender_ = std::move(sender);
  serializer_ = std::move(serializer);
  cfg_mgr_ = std::move(cfg_mgr);
}

std::optional<Peer> PeerApi::get(const GetPeerRequest& req) const {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::get);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peer");
  u.params().set("publicKey", req.public_key);

  http_req.target = u.buffer();

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::not_found))
	return std::nullopt;
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  return serializer_->deserialize(res.body);
}

std::optional<std::vector<Peer>> PeerApi::getAll(const GetAllPeerRequest& req) const {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::get);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peers");
  http_req.target = u.buffer();

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  json::value jv = json::parse(res.body);
  const auto& o = jv.as_object();
  const auto& peers = o.at("peers").as_array();
  if (peers.size() == 0)
	return std::nullopt;

  std::vector<Peer> v;
  v.reserve(peers.size());
  for (const auto& peer : peers) {
	auto p = json::value_to<Peer>(peer);
	v.emplace_back(p);
  }

  return v;
}

CreatePeerResponse PeerApi::create(const CreatePeerRequest& req) {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::post);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peer");
  http_req.target = u.buffer();

  json::object o{
      {"generatePresharedKey", req.generate_preshared_key},
      {"allowedIps", json::array(req.allowed_ips.begin(), req.allowed_ips.end())},
      {"persistentKeepalive", req.persistent_keepalive},
      {"description", req.description}
  };

  http_req.body = json::serialize(o);

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::bad_request))
	throw std::invalid_argument("Invalid data provided or interface ran out of peers capacity");
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  return json::value_to<CreatePeerResponse>(json::parse(http_req.body));
}

void PeerApi::update(const UpdatePeerRequest& req) {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::patch);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peer");
  u.params().set("publicKey", req.public_key);
  http_req.target = u.path();

  json::object o;
  if (req.preshared_key)
	o["presharedKey"] = *req.preshared_key;
  if (req.persistent_keepalive)
	o["persistentKeepalive"] = *req.persistent_keepalive;
  if (req.description && req.description->size())
	o["description"] = *req.description;
  if (req.add_allowedips && req.add_allowedips->size())
	o["addAllowedIps"] = json::array(req.add_allowedips->begin(), req.add_allowedips->end());
  if (req.remove_allowedips && req.remove_allowedips->size())
	o["removeAllopwedIps"] =
	    json::array(req.remove_allowedips->begin(), req.remove_allowedips->end());

  http_req.body = json::serialize(o);

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::bad_request))
	throw std::invalid_argument("Malformed query or overlapping tunnel address provided");
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");
}

void PeerApi::softDelete(const DeletePeerRequest& req) {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::delete_);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peer");
  u.params().set("publicKey", req.public_key);
  http_req.target = u.buffer();

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::bad_gateway))
	throw std::invalid_argument("Invalid data provided");
  if (res.status == static_cast<int>(http::status::not_found))
	throw std::runtime_error("Target peer not found");
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");
}

RotatePeerPublicKeyResponse PeerApi::rotatePublicKey(const RotatePeerPublicKeyRequest& req) {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::patch);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peer/rotate");
  u.params().set("oldPublicKey", req.old_key);
  http_req.target = u.buffer();

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::not_found))
	throw std::runtime_error("Peer not found");
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  return json::value_to<RotatePeerPublicKeyResponse>(json::parse(res.body));
}

std::string PeerApi::getConfig(const GetPeerConfigRequest& req) const {
  if (!req.endpoint && req.iface && !ensureConfigOpenedImpl(*req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::get);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/peer/config");
  u.params().set("publicKey", req.public_key);
  if (req.public_endpoint)
	u.params().set("publicEndpoint", *req.public_endpoint);
  http_req.target = u.buffer();

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::not_found))
	throw std::runtime_error("Peer not found");
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  return json::parse(res.body).as_object()["config"].as_string().c_str();
}

bool PeerApi::ensureConfigOpenedImpl(const std::string& iface) const {
  const auto path = std::format("/etc/wgforge/{}.conf", iface);
  if (cfg_mgr_->path() == path)
	return true;

  cfg_mgr_->close();
  if (cfg_mgr_->open(path, true))
	return false;
  return true;
}

urls::url PeerApi::getEncodedUrlImpl(std::optional<std::string> endpoint) const {
  urls::url u;
  if (endpoint) {
	const auto colon_pos = endpoint->find(':');
	if (colon_pos == std::string::npos)
	  throw std::invalid_argument("Invalid endpoint");
	try {
	  u.set_scheme("http")
	      .set_host(endpoint->substr(0, colon_pos))
	      .set_port(endpoint->substr(colon_pos + 1));
	} catch (const boost::system::system_error&) {
	  throw std::invalid_argument(std::format("Invalid remote host endpoint: {}", *endpoint));
	}
	return u;
  }

  ConfigReadRegister::ReadReturnCode rc;
  cfg_mgr_->beginTransaction();

  auto addr_opt = cfg_mgr_->tryRead<std::string>("Server", "Address", &rc);
  if (rc) {
	cfg_mgr_->rollBack();
	throw std::runtime_error(
	    "Failed to read server configuration field \"Address\". Check config rights or field "
	    "existance");
  }

  auto port_opt = cfg_mgr_->tryRead<uint32_t>("Server", "ListenPort", &rc);
  if (rc) {
	cfg_mgr_->rollBack();
	throw std::runtime_error(
	    "Failed to read server configuration field \"ListenPort\". Check config right or field "
	    "existance");
  }

  if (*port_opt > std::numeric_limits<in_port_t>::max() && *port_opt == 0)
	throw std::invalid_argument("Invalid ListenPort value for [Server]");

  u.set_scheme("http").set_encoded_host(*addr_opt).set_port_number(*port_opt);
  return u;
}

}  // namespace WgForge
