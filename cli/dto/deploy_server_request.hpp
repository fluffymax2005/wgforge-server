/*
 * deploy_server_request.hpp
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

#ifndef WGFORGE_CLI_DEPLOY_SERVER_REQUEST_HPP
#define WGFORGE_CLI_DEPLOY_SERVER_REQUEST_HPP

#include <string>

namespace WgForge {

struct DeployServerRequest {
  std::string image = "fluffymax2005/wgforge-server:latest";
  std::string iface = "wg0";
  std::string config_path = "/etc/wgforge";
  std::string data_path = "/var/lib/wgforge";
  std::string container_name = "wgforge-server_" + iface;

  std::string ssh_host = "";  // user@host or host
  int ssh_port = 22;
  std::string ssh_key = "";       // explicit path to private key
  std::string ssh_password = "";  // password in case of keys are missing
};

}  // namespace WgForge

#endif  // WGFORGE_CLI_DEPLOY_SERVER_REQUEST_HPP
