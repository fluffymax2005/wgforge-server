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

InterfaceConfig InterfaceApi::get(const std::string& iface) const {
  if (!ensureConfigOpenedImpl(iface))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest req{};
  req.method = http::to_string(http::verb::get);

  auto u = getEncodedUrlImpl();
  u.set_path("/interface");
  req.target = u.buffer();

  auto res = sender_->send(req);
  if (res.status == static_cast<int>(http::status::internal_server_error))
	throw std::runtime_error("Internal server error occured while processing request");

  return serializer_->deserialize(res.body);
}

void InterfaceApi::update(const InterfaceConfig& iface) {
  if (!ensureConfigOpenedImpl(iface.name))
	throw std::runtime_error("Failed to open config to read daemon details");

  HttpRequest req{};
  req.method = http::to_string(http::verb::patch);

  auto u = getEncodedUrlImpl();
  u.set_path("/interface");
  req.target = u.buffer();

  req.body = json::serialize(json::value_from(iface));

  auto res = sender_->send(req);
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

urls::url InterfaceApi::getEncodedUrlImpl() const {
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

  urls::url u;
  u.set_scheme("http").set_encoded_host(*addr_opt).set_port_number(*port_opt);
  return u;
}

}  // namespace WgForge
