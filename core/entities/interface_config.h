/*
 * interface_config.h
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

#ifndef __WGFORGE_INTERFACE_CONFIG_H__
#define __WGFORGE_INTERFACE_CONFIG_H__

#include <net/if.h>
#include <netinet/in.h>

#include <wgallowedip.hpp>

#include "wgforge.h"

namespace WgForge {

/**
 * @brief Wireguard iface configuration
 */
struct InterfaceConfig {
  char addr[INET_ADDRSTRLEN];  ///< IPv4 address
  WgPrivateKey private_key;    ///< Private key
  char name[IFNAMSIZ];         ///< Name
  in_port_t listenPort;        ///< Listen port
  Protocol proto_type;         ///< Protocol type listening on. Reserved for future
  bool checkSubnet;            ///< Whether daemon check subnet occupation on startup
};

}  // namespace WgForge


#endif  // __WGFORGE_INTERFACE_CONFIG_H__
