/*
 * creat_peer_request.hpp
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

#ifndef __WGFORGE_CREATE_PEER_REQUEST_H__
#define __WGFORGE_CREATE_PEER_REQUEST_H__

#include <boost/json.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "json_extensions.hpp"
#include "wgforge.h"

namespace WgForge {

namespace json = boost::json;

struct CreatePeerRequest {
  bool generate_preshared_key = true;
  uint16_t persistent_keepalive;
  std::vector<std::string> allowed_ips;

  CreatePeerRequest(const json::object& o) {
	auto gen_key_opt = JsonExtensions::get_optional_field<bool>(o, "generatePresharedKey", false);
	generate_preshared_key = gen_key_opt.value_or(true);

	auto keepalive = JsonExtensions::get_optional_field<int64_t>(o, "persistentKeepalive").value();
	if (keepalive <= 0 || keepalive > std::numeric_limits<uint16_t>::max())
	  throw EndpointInvalidArgument("persistentKeepalive must be in range [1; 65535]");
	persistent_keepalive = keepalive;

	auto ips = JsonExtensions::get_optional_field<json::array>(o, "allowedIps").value();
	allowed_ips.reserve(ips.size());

	for (auto i = 0; i < ips.size(); i++) {
	  try {
		std::string ip = ips[i].as_string().c_str();
		if (!WgAllowedIP::validate(ip))
		  throw std::invalid_argument("");
		allowed_ips.emplace_back(std::move(ip));
	  } catch (const boost::system::system_error&) {
		throw EndpointInvalidArgument(std::format("Invalid allowed ip at position: {}", i));
	  } catch (const std::invalid_argument&) {
		throw EndpointInvalidArgument(std::format("Invalid allowed ip at position: {}", i));
	  }
	}
  }
};

}  // namespace WgForge

#endif  // __WGFORGE_CREATE_PEER_REQUEST_H__
