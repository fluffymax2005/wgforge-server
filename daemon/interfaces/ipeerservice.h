/*
 * ipeerservice.h
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

#ifndef __WGFORGE_IPEERSERVICE_H__
#define __WGFORGE_IPEERSERVICE_H__

#include <boost/json/object.hpp>
#include <future>
#include <optional>
#include <vector>

#include "allowedip.hxx"
#include "create_peer_request.hpp"
#include "create_peer_response.hpp"
#include "peer.hxx"

namespace WgForge {

class IPeerService {
public:
  using Key = int32_t;

  virtual ~IPeerService() = default;

  virtual std::future<std::optional<Peer>> getPeerAsync(const Key& key) const = 0;
  virtual std::future<std::optional<std::vector<Peer>>> getAllAsync() const = 0;
  virtual std::future<CreatePeerResponse> createPeerAsync(const CreatePeerRequest& req) = 0;
  virtual std::future<void> softDeleteAsync(const Key& key) = 0;
  virtual std::future<void> hardDeleteAsync(const Key& key) = 0;

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
};

}  // namespace WgForge

#endif  // __WGFORGE_IPEERSERVICE_H__
