/*
 * inet_utils.h
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

#ifndef __WGFORGE_INETUTILS_H__
#define __WGFORGE_INETUTILS_H__

#include "wgforge.h"

namespace WgForge {

class InetUtils {
public:
  // Assign IP address to Wireguard interface
  static void assignIpAddress(const InterfaceConfig& config);

  // Check whether port is free. Default is TCP, UDP otherwise checked
  static bool isPortFree(std::string_view ip, in_port_t port, bool isTCP = true);

  // Find first available lo address for given port
  static std::string findFreeLoopbackAddress(in_port_t port, bool isTCP = true);

  static std::string findFreeClientTunnelCIDR(std::string_view gatewayCIDR,
                                              const std::vector<std::string>& cidrs);

  static std::string getGatewayCIDR(std::string_view net_cidr);

private:
  static std::string toCIDR4Impl(uint32_t ip_host, uint8_t prefix);
  static std::optional<std::tuple<uint32_t, uint32_t, uint8_t>> parseCIDR4Impl(
      std::string_view cidr);
};

}  // namespace WgForge

#endif
