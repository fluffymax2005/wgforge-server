/*
 * rotate_peer_public_key_request.h
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

#ifndef __WGFORGE_ROTATE_PEER_PUBLIC_KEY_REQUEST_H__
#define __WGFORGE_ROTATE_PEER_PUBLIC_KEY_REQUEST_H__

#include <string>
#include <wgpublickey.hpp>

#include "defines.h"
#include "endpoint_invalid_argument.h"
#include "wgforge.h"

namespace WgForge {

struct RotatePeerPublicKeyRequest {
  std::string old_key;

  RotatePeerPublicKeyRequest(std::string old_key) {
	if (!WgPublicKey::validateB64StringKey(old_key))
	  throw EndpointInvalidArgument("Invalid old public key");

	this->old_key = std::move(old_key);
  }
};

}  // namespace WgForge


#endif  // __WGFORGE_ROTATE_PEER_PUBLIC_KEY_REQUEST_H__
