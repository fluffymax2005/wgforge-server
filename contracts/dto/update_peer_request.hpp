/*
 * update_peer_request.hpp
 * wgforge-contracts — contracts for wgforge-server (C) and wgforge-cli  (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-contracts is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-contracts is distributed in the hope that it will be useful,
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

namespace WgForge {

struct UpdatePeerRequest {
  std::optional<std::string> endpoint;
  std::optional<std::string> iface;
  std::string public_key;
  std::optional<std::string> preshared_key;
  std::optional<Peer::Status> status;
  std::optional<std::string> description;
  std::optional<uint16_t> persistent_keepalive;
  std::optional<std::vector<std::string>> add_allowedips;
  std::optional<std::vector<std::string>> remove_allowedips;

  UpdatePeerRequest() = default;

  UpdatePeerRequest(const json::object& j);

  [[nodiscard("Peer update necessity should not be ignored")]]
  bool requiresQuery() const noexcept {
	return endpoint || iface || preshared_key || status || description || persistent_keepalive ||
	       add_allowedips || remove_allowedips;
  }
};

}  // namespace WgForge


#endif  // __WGFORGE_UPDATE_PEER_REQUEST_HPP__
