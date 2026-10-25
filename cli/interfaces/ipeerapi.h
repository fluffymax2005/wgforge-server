/*
 * ipeerapi.h
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_IPEERAPI_H
#define WGFORGE_IPEERAPI_H

#include <optional>
#include <peer.hxx>
#include <rotate_peer_public_key_request.hpp>
#include <rotate_peer_public_key_response.hpp>
#include <string>
#include <vector>

#include "create_peer_request.hpp"
#include "create_peer_response.hpp"
#include "delete_peer_request.hpp"
#include "get_all_peer_request.hpp"
#include "get_peer_config_request.hpp"
#include "get_peer_request.hpp"
#include "update_peer_request.hpp"

namespace WgForge {

class IPeerApi {
public:
  virtual ~IPeerApi() = default;

  virtual std::optional<Peer> get(const GetPeerRequest& req) const = 0;
  virtual std::optional<std::vector<Peer>> getAll(const GetAllPeerRequest& req) const = 0;
  virtual CreatePeerResponse create(const CreatePeerRequest& req) = 0;
  virtual void update(const UpdatePeerRequest& req) = 0;
  virtual void softDelete(const DeletePeerRequest& req) = 0;

  virtual RotatePeerPublicKeyResponse rotatePublicKey(const RotatePeerPublicKeyRequest& req) = 0;
  virtual std::string getConfig(const GetPeerConfigRequest& req) const = 0;
};

}  // namespace WgForge

#endif  // WGFORGE_IPEERAPI_H
