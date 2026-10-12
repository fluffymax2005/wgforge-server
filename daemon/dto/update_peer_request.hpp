/*
 * update_peer_request.hpp
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

#ifndef __WGFORGE_UPDATE_PEER_REQUEST_HPP__
#define __WGFORGE_UPDATE_PEER_REQUEST_HPP__

#include <boost/json.hpp>
#include <cstdint>
#include <optional>
#include <peer.hxx>
#include <string>
#include <vector>

#include "defines.h"
#include "endpoint_invalid_argument.h"
#include "json_extensions.hpp"

namespace WgForge {

struct UpdatePeerRequest {
  std::string public_key;
  std::optional<std::string> preshared_key;
  std::optional<uint16_t> persistent_keepalive;
  std::optional<std::string> description;
  std::optional<std::vector<std::string>> add_allowedips;
  std::optional<std::vector<std::string>> remove_allowedips;
  std::optional<Peer::Status> status;

  UpdatePeerRequest(const json::object& j) {
	auto key = JsonExtensions::get_optional_field<std::string_view>(j, "publicKey").value();
	if (!WgPublicKey::validateB64StringKey(key))
	  throw EndpointInvalidArgument("Invalid public key");
	public_key = key;

	auto preshared_key_opt =
	    JsonExtensions::get_optional_field<std::string_view>(j, "presharedKey", false);
	if (preshared_key_opt) {
	  const std::string_view preshared_key_sv = preshared_key_opt->data();
	  if (!WgPresharedKey::validateB64StringKey(preshared_key_sv))
		throw EndpointInvalidArgument("Invalid preshared key");
	  preshared_key = preshared_key_sv;
	}

	auto persistent_keepalive_opt =
	    JsonExtensions::get_optional_field<int64_t>(j, "persistentKeepalive", false);
	if (persistent_keepalive_opt) {
	  auto persistent_keepalive = persistent_keepalive_opt.value();
	  if (persistent_keepalive <= 0 ||
	      persistent_keepalive >=
	          std::numeric_limits<decltype(UpdatePeerRequest::persistent_keepalive)>::max())
		throw EndpointInvalidArgument(
		    "Incorrect persistent keepalive. It must be in range [1; 65535]");
	  persistent_keepalive = persistent_keepalive;
	}

	auto description_opt =
	    JsonExtensions::get_optional_field<std::string_view>(j, "description", false);
	if (description_opt) {
	  description = description_opt.value();
	}

	auto status_opt = JsonExtensions::get_optional_field<int64_t>(j, "status", false);
	if (status_opt) {
	  auto st = status_opt.value();
	  if (!(st >= static_cast<int64_t>(Peer::Status::Active) &&
	        st <= static_cast<int64_t>(Peer::Status::Archived)))
		throw EndpointInvalidArgument("Invalid status");
	  status = static_cast<Peer::Status>(st);
	}

	auto getAllowedIpsVec = [&](std::string_view field_name) -> std::vector<std::string> {
	  auto allowedips_opt = JsonExtensions::get_optional_field<json::array>(j, field_name, false);
	  std::vector<std::string> allowedips_vec;
	  if (allowedips_opt) {
		auto allowedips = allowedips_opt.value();
		for (const auto& ip : allowedips) {
		  if (!ip.is_string())
			throw EndpointInvalidArgument(std::format("Invalid \"{}\"", field_name));
		  std::string_view ip_sv = ip.as_string().data();
		  if (!WgAllowedIP::validate(ip_sv.data()))
			throw EndpointInvalidArgument(
			    std::format("Incorrect element in \"{}\": \"{}\"", field_name, ip_sv));
		  allowedips_vec.emplace_back(ip_sv);
		}
	  }

	  return allowedips_vec;
	};

	auto add_ips = getAllowedIpsVec("addAllowedIps");
	if (add_ips.size())
	  add_allowedips = std::move(add_ips);

	auto remove_ips = getAllowedIpsVec("removeAllowedIps");
	if (remove_ips.size())
	  remove_allowedips = std::move(remove_ips);
  }
};

}  // namespace WgForge


#endif  // __WGFORGE_UPDATE_PEER_REQUEST_HPP__
