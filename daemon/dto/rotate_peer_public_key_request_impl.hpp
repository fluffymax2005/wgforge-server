/*
 * rotate_peer_public_key_request_impl.hpp
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

#ifndef WGFORGE_ROTATE_PEER_REQUEST_IMPL_HPP
#define WGFORGE_ROTATE_PEER_REQUEST_IMPL_HPP

#include <rotate_peer_public_key_request.h>

#include "endpoint_invalid_argument.h"
#include "wgforge.h"

namespace WgForge {

inline RotatePeerPublicKeyRequest::RotatePeerPublicKeyRequest(std::string iface,
                                                              std::string old_key) {
  if (!WgPublicKey::validateB64StringKey(old_key))
	throw EndpointInvalidArgument("Invalid old public key");

  this->old_key = std::move(old_key);
  this->iface = std::move(iface);
}

}  // namespace WgForge

#endif  // WGFORGE_ROTATE_PEER_REQUEST_IMPL_HPP
