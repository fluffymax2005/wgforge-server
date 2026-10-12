/*
 * wireguard_config.h
 * wgforge-core — Core library for wgforge-server (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-core is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-core. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __WGFORGE_WIREGUARD_CONFIG_H__
#define __WGFORGE_WIREGUARD_CONFIG_H__

#include <netinet/in.h>

namespace WgForge {

struct WireguardConfig {
  char public_endpoint[INET_ADDRSTRLEN + 6];  // :<port_number>
};

}  // namespace WgForge


#endif  // __WGFORGE_WIREGUARD_CONFIG_H__
