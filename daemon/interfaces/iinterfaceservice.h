/*
 * iinterfaceservice.h
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

#ifndef WGFORGE_IINTERFACESERVICE_H
#define WGFORGE_IINTERFACESERVICE_H

#include "interface_info_response.hpp"
#include "interface_update_request.hpp"

namespace WgForge {

class IInterfaceService {
public:
  virtual ~IInterfaceService() = default;

  virtual InterfaceInfoResponse getInfo() const = 0;
  virtual void updateConfig(const InterfaceUpdateRequest& req) = 0;
};

}  // namespace WgForge


#endif  // WGFORGE_IINTERFACESERVICE_H
