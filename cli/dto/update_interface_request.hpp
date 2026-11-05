/*
 * update_interface_request.h
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

#ifndef WGFORGE_UPDATE_INTERFACE_REQUEST_HPP
#define WGFORGE_UPDATE_INTERFACE_REQUEST_HPP

#include <netinet/in.h>

#include <optional>
#include <string>

namespace WgForge {

struct UpdateInterfaceRequest {
  std::string iface;
  std::optional<std::string> endpoint;
  std::optional<std::string> address;
  std::optional<std::string> private_key;
  std::optional<in_port_t> listen_port;
  std::optional<bool> check_subnet;

  [[nodiscard("Interface update neccesity should not be ignored")]]
  bool requiresQuery() const noexcept {
	return endpoint || address || private_key || listen_port || check_subnet;
  }
};

}  // namespace WgForge

#endif  // WGFORGE_UPDATE_INTERFACE_REQUEST_HPP
