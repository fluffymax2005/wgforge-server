/*
 * update_peer_request_impl.hpp
 * wgforge-server — backend to manage wireguard
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

#ifndef WGFORGE_UPDATE_PEER_REQUEST_IMPL_HPP
#define WGFORGE_UPDATE_PEER_REQUEST_IMPL_HPP

#include <update_peer_request.hpp>

#include "endpoint_invalid_argument.h"
#include "json_extensions.hpp"
#include "wgforge.h"

namespace WgForge {

inline UpdatePeerRequest::UpdatePeerRequest(const json::object& j) {
  auto public_key_opt = JsonExtensions::get_optional_field<std::string_view>(j, "publicKey");
  if (!WgPublicKey::validateB64StringKey(*public_key_opt))
	throw EndpointInvalidArgument("Invalid public key");
  public_key = *public_key_opt;

  if (auto preshared_key_opt =
          JsonExtensions::get_optional_field<std::string_view>(j, "presharedKey", false)) {
	if (!WgPresharedKey::validateB64StringKey(*preshared_key_opt))
	  throw EndpointInvalidArgument("Invalid preshared key");
	preshared_key = *preshared_key_opt;
  }

  if (auto status_opt = JsonExtensions::get_optional_field<int64_t>(j, "status", false)) {
	if (*status_opt <= static_cast<int64_t>(Peer::Status::Active) ||
	    *status_opt >= static_cast<int64_t>(Peer::Status::Archived))
	  throw EndpointInvalidArgument("Invalid status");
	status = static_cast<Peer::Status>(*status_opt);
  }

  if (auto description_opt =
          JsonExtensions::get_optional_field<std::string_view>(j, "description", false))
	description = *description_opt;

  if (auto persistent_keepalive_opt =
          JsonExtensions::get_optional_field<int64_t>(j, "persistentKeepalive", false)) {
	if (*persistent_keepalive_opt == 0)
	  throw EndpointInvalidArgument("Invalid persistent keepalive");
	persistent_keepalive = *persistent_keepalive_opt;
  }

  if (auto allowedips_arr = JsonExtensions::get_optional_field<json::array>(j, "addAllowedIps")) {
	add_allowedips->reserve(allowedips_arr->size());
	for (const auto& ip_val : *allowedips_arr) {
	  auto ip = ip_val.get_string();
	  if (!WgAllowedIP::validate(ip.c_str()))
		throw EndpointInvalidArgument(std::format("Invalid allowed ip: {}", ip));
	  add_allowedips->push_back(ip.c_str());
	}
  }

  if (auto allowedips_arr =
          JsonExtensions::get_optional_field<json::array>(j, "removeAllowedIps")) {
	remove_allowedips->reserve(allowedips_arr->size());
	for (const auto& ip_val : *allowedips_arr) {
	  auto ip = ip_val.get_string();
	  if (!WgAllowedIP::validate(ip.c_str()))
		throw EndpointInvalidArgument(std::format("Invalid allowed ip: {}", ip));
	  remove_allowedips->push_back(ip.c_str());
	}
  }
}

}  // namespace WgForge

#endif  // WGFORGE_UPDATE_PEER_REQUEST_IMPL_HPP
