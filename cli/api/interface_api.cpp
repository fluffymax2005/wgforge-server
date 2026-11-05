/*
 * interface_api.cpp
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

#include "interface_api.h"

#include <netinet/in.h>

#include <boost/beast.hpp>

namespace WgForge {

namespace beast = boost::beast;
namespace http = beast::http;

InterfaceApi::InterfaceApi(std::shared_ptr<IHttpSender> sender,
                           std::shared_ptr<ISerializer<InterfaceConfig>> serializer,
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

InterfaceConfig InterfaceApi::get(const GetInterfaceConfigRequest& req) const {
  if (req.endpoint && !ensureConfigOpenedImpl(req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::get);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/interface");
  http_req.target = u.buffer();

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  return serializer_->deserialize(res.body);
}

void InterfaceApi::update(const UpdateInterfaceRequest& req) {
  if (req.endpoint && !ensureConfigOpenedImpl(req.iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest http_req{};
  http_req.method = http::to_string(http::verb::patch);

  auto u = getEncodedUrlImpl(req.endpoint);
  u.set_path("/interface");
  http_req.target = u.buffer();

  json::object o{
      {"privateKey", req.private_key.value_or("")},
      {   "address",     req.address.value_or("")},
      {"listenPort",              req.listen_port}
  };

  http_req.body = json::serialize(json::value_from(o));

  auto res = sender_->send(http_req);
  if (res.status == static_cast<int>(http::status::bad_request))
	throw std::invalid_argument("Invalid data provided");
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");
}

bool InterfaceApi::ensureConfigOpenedImpl(const std::string& iface) const {
  const auto path = std::format("/etc/wgforge/{}.conf", iface);
  if (cfg_mgr_->path() == path)
	return true;

  cfg_mgr_->close();
  if (cfg_mgr_->open(path, true))
	return false;
  return true;
}

urls::url InterfaceApi::getEncodedUrlImpl(std::optional<std::string> endpoint) const {
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
