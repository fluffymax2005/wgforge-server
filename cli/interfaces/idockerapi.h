/*
 * idockerapi.h
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

#ifndef WGFORGE_CLI_IDOCKERAPI_H
#define WGFORGE_CLI_IDOCKERAPI_H

#include <ostream>
#include <string>

#include "deploy_server_request.hpp"

namespace WgForge {

class IDockerApi {
public:
  virtual ~IDockerApi() = default;

  virtual void ping() = 0;
  virtual void pullImage(const std::string& image) = 0;
  virtual void createContainer(const DeployServerRequest& req) = 0;
  virtual void startContainer(const std::string& container_name) = 0;
  virtual void stopAndRemoveContainer(const std::string& container_name) = 0;
};

}  // namespace WgForge

#endif  // WGFORGE_CLI_IDOCKERAPI_H
