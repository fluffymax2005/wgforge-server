/*
 * get_all_peer_request.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
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

#ifndef WGFORGE_GET_ALL_PEER_REQUEST_HPP
#define WGFORGE_GET_ALL_PEER_REQUEST_HPP

#include <optional>
#include <string>
#include <vector>

#include "display.h"

namespace WgForge {

struct GetAllPeerRequest {
  std::optional<std::string> endpoint;
  std::optional<std::string> iface;
  Display display;
  std::vector<std::string> cols;
  bool short_keys = false;

  [[nodiscard("Query neccesity should not be ignored")]]
  bool requiredQuery() const noexcept {
	return endpoint || iface;
  }
};

}  // namespace WgForge

#endif  // WGFORGE_GET_ALL_PEER_REQUEST_HPP
