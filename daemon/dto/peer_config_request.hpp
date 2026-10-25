/*
 * peer_config_request.hpp
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

#ifndef WGFORGE_PEER_CONFIG_REQUEST_HPP
#define WGFORGE_PEER_CONFIG_REQUEST_HPP

#include <optional>
#include <string>
#include <wgendpoint.hpp>
#include <wgpublickey.hpp>

#include "defines.h"
#include "endpoint_invalid_argument.h"

namespace WgForge {

struct PeerConfigRequest {
  std::string public_key;
  std::optional<std::string> public_endpoint;

  PeerConfigRequest(std::string public_key, std::string public_endpoint = "") {
	if (!WgPublicKey::validateB64StringKey(public_key))
	  throw EndpointInvalidArgument("Invalid public key");
	if (public_endpoint.length()) {
	  if (!WgEndpoint::validate(public_endpoint))
		throw EndpointInvalidArgument("Invalid public endpoint");
	  this->public_endpoint = public_endpoint;
	}

	this->public_key = std::move(public_key);
  }
};

}  // namespace WgForge


#endif  // WGFORGE_PEER_CONFIG_REQUEST_HPP
