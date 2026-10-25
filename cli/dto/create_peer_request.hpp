/*
 * create_peer_request.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
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

#ifndef WGFORGE_CREATE_PEER_REQUEST_HPP
#define WGFORGE_CREATE_PEER_REQUEST_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace WgForge {

struct CreatePeerRequest {
  std::optional<std::string> endpoint;
  std::optional<std::string> iface;
  bool generate_preshared_key = true;
  std::vector<std::string> allowed_ips;
  std::uint16_t persistent_keepalive = 25;
  std::optional<std::string> description;

  [[nodiscard("Query neccesity should not be ignored")]]
  bool requiresQuery() const noexcept {
	return endpoint || iface;
  }
};

}  // namespace WgForge

#endif  // WGFORGE_CREATE_PEER_REQUEST_HPP
