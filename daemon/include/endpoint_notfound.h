/*
 * endpoint_notfound.h
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

#ifndef __WGFORGE_ENDPOINT_NOTFOUND_H__
#define __WGFORGE_ENDPOINT_NOTFOUND_H__

#include <stdexcept>

namespace WgForge {

class EndpointNotFound : public std::invalid_argument {
public:
  EndpointNotFound(const std::string& msg) : std::invalid_argument(msg) {}

  EndpointNotFound(const char* msg) : std::invalid_argument(msg) {}
};

}  // namespace WgForge

#endif  //  __WGFORGE_ENDPOINT_NOTFOUND_H__
