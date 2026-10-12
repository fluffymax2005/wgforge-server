/*
 * ipeerservice.h
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

#ifndef __WGFORGE_IPEERSERVICE_H__
#define __WGFORGE_IPEERSERVICE_H__

#include <boost/json/object.hpp>
#include <future>
#include <optional>
#include <string_view>
#include <vector>

#include "allowedip.hxx"
#include "create_peer_request.hpp"
#include "create_peer_response.hpp"
#include "peer.hxx"
#include "peer_config_request.hpp"
#include "rotate_peer_public_key_request.hpp"
#include "update_peer_request.hpp"

namespace WgForge {

class IPeerServiceAsync {
public:
  using Key = int32_t;

  virtual ~IPeerServiceAsync() = default;

  virtual std::future<std::optional<Peer>> getPeerAsync(const Key& key) const = 0;
  virtual std::future<std::optional<Peer>> getPeerByPublicKey(std::string_view key) const = 0;
  virtual std::future<std::optional<std::vector<Peer>>> getAllAsync() const = 0;
  virtual std::future<CreatePeerResponse> createPeerAsync(const CreatePeerRequest& req) = 0;
  virtual std::future<void> softDeleteAsync(const Key& key) = 0;
  virtual std::future<void> hardDeleteAsync(const Key& key) = 0;

  virtual std::future<void> updateAsync(const UpdatePeerRequest& req) = 0;

  virtual std::future<std::optional<AllowedIP>> getAllowedIP(const Key& key) const = 0;
  virtual std::future<std::optional<std::vector<AllowedIP>>> getAllAllowedIPs(
      const Key& key) const = 0;
  virtual std::future<void> addAllowedIPAsync(const Key& key, std::string_view allowedip) = 0;
  virtual std::future<void> addAllowedIPsAsync(const Key& key,
                                               const std::vector<std::string>& allowedips) = 0;
  virtual std::future<void> softDeleteAllowedIPAsync(const Key& key,
                                                     std::string_view allowedip) = 0;
  virtual std::future<void> softDeleteAllowedIPsAsync(
      const Key& key, const std::vector<std::string>& allowedips) = 0;

  virtual std::future<void> rotatePublicKeyAsync(const RotatePeerPublicKeyRequest& req) = 0;
  virtual std::future<std::string> getPeerConfigAsync(const PeerConfigRequest& req) = 0;
};

}  // namespace WgForge

#endif  // __WGFORGE_IPEERSERVICE_H__
