/*
 * ipeermanager.h
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_IPEERMANAGER_H
#define WGFORGE_IPEERMANAGER_H

#include <create_peer_response.hpp>
#include <optional>
#include <peer.hxx>
#include <rotate_peer_public_key_request.hpp>
#include <rotate_peer_public_key_response.hpp>
#include <string>
#include <update_peer_request.hpp>
#include <vector>

#include "create_peer_request.hpp"
#include "delete_peer_request.hpp"
#include "get_all_peer_request.hpp"
#include "get_peer_config_request.hpp"
#include "get_peer_request.hpp"

namespace WgForge {

class IPeerManager {
public:
  virtual ~IPeerManager() = default;

  virtual std::optional<std::vector<Peer>> getAll(const GetAllPeerRequest& req) const = 0;
  virtual std::optional<Peer> getByPublicKey(const GetPeerRequest& req) const = 0;
  virtual CreatePeerResponse create(const CreatePeerRequest& req) = 0;
  virtual void updateByPublicKey(const UpdatePeerRequest& req) = 0;
  virtual void deleteByPublicKey(const DeletePeerRequest& req) = 0;

  virtual RotatePeerPublicKeyResponse rotatePublicKey(const RotatePeerPublicKeyRequest& req) = 0;
  virtual std::string getConfig(const GetPeerConfigRequest& req) const = 0;
};

}  // namespace WgForge


#endif  // WGFORGE_IPEERMANAGER_H
