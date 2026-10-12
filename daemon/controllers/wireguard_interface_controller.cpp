/*
 * wireguard_interface_controller.cpp
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

#include "wireguard_interface_controller.h"

#include "endpoint_notfound.h"

namespace WgForge {

WireguardInterfaceController::WireguardInterfaceController(
    std::shared_ptr<ILogger> logger, std::shared_ptr<ConfigManager> config_mgr,
    std::shared_ptr<IInterfaceService> iface_service) {
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");
  if (config_mgr == nullptr)
	throw std::invalid_argument("Config manager is nullptr");
  if (iface_service == nullptr)
	throw std::invalid_argument("Interface service is nullptr");

  logger_ = std::move(logger);
  config_mgr_ = std::move(config_mgr);
  iface_service_ = std::move(iface_service);
}

void WireguardInterfaceController::getInfo(Request req, Response res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	auto info = iface_service_->getInfo();

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(json::value_from(info));
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void WireguardInterfaceController::updateConfig(Request req, Response res) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "ONLY PATCH";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	auto j = json::parse(req.body()).as_object();
	iface_service_->updateConfig(j);

	res.result(http::status::no_content);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void WireguardInterfaceController::handleException(std::exception_ptr ep, Request req,
                                                   Response& res) {
  try {
	if (ep)
	  std::rethrow_exception(ep);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const boost::system::system_error& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::runtime_error& e) {
	auto path = std::string(req.target());
	auto method = std::string(req.method_string());
	logger_->error(std::format("{} [{}]: {}", path, method, e.what()));

	res.result(http::status::internal_server_error);
  } catch (const EndpointNotFound& e) {
	json::object o;
	o["error"] = e.what();
	res.result(http::status::not_found);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const std::exception& e) {
	logger_->error(std::format("Unexpected error: {}", e.what()));
	res.result(http::status::internal_server_error);
	res.body() = "Internal server error";
  }
}

}  // namespace WgForge
