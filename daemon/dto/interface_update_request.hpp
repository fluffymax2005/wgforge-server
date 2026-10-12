/*
 * interface_update_request.hpp
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

#ifndef __WGFORGE_INTERFACE_UPDATE_REQUEST_HPP__
#define __WGFORGE_INTERFACE_UPDATE_REQUEST_HPP__

#include <netinet/in.h>

#include <boost/json.hpp>
#include <limits>
#include <optional>
#include <string>

#include "defines.h"
#include "endpoint_invalid_argument.h"
#include "json_extensions.hpp"
#include "wgforge.h"

namespace WgForge {

struct InterfaceUpdateRequest {
  std::optional<std::string> private_key;
  std::optional<std::string> address;
  std::optional<in_port_t> listen_port;

  InterfaceUpdateRequest(const json::object& j) {
	auto key_opt = JsonExtensions::get_optional_field<std::string_view>(j, "privateKey", false);
	if (key_opt) {
	  if (!WgPrivateKey::validateB64StringKey(key_opt.value()))
		throw EndpointInvalidArgument("Invalid private key");
	  private_key = std::string(key_opt.value());
	}

	auto addr_opt = JsonExtensions::get_optional_field<std::string_view>(j, "address", false);
	if (addr_opt) {
	  if (!WgAllowedIP::validate(addr_opt.value().data()))
		throw EndpointInvalidArgument("Invalid listen address");
	  address = std::string(addr_opt.value());
	}

	auto port_opt = JsonExtensions::get_optional_field<int64_t>(j, "listenPort", false);
	if (port_opt) {
	  auto port = port_opt.value();
	  if (port < 0 ||
	      port > std::numeric_limits<decltype(InterfaceUpdateRequest::listen_port)>::max())
		throw EndpointInvalidArgument("Invalid listen port");
	  listen_port = port;
	}
  }
};

}  // namespace WgForge

#endif  // __WGFORGE_INTERFACE_UPDATE_REQUEST_HPP__
