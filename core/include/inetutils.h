/*
 * inet_utils.h
 * wgforge-core — Core library for wgforge-server (C) and wgforge-cli (C)
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

#ifndef WGFORGE_INETUTILS_H
#define WGFORGE_INETUTILS_H

#include "interface_config.hpp"

namespace WgForge {

/**
 * @brief Auxiliary class providing inet utilities. Currenly supports **only** IPv4.
 */
class InetUtils {
public:
  /** 
   * @brief Assign IP address to Wireguard interface
   * @param config target interface configuration.
   * @throw std::invalid_argument if config is invalid
   * @throw std::runtime_error if failed to serve request
   */
  static void assignIpAddress(const InterfaceConfig& config);

  /** 
   * @brief Check whether port is free.
   * @param ip string ip representation
   * @param port to to check
   * @param isTCP whether to check TCP socket type, otherwise UDP is checked
   * @retval true if port is free
   * @retval false otherwise
   * @throw std::invalid_argument if invalid ip provided
   * @throw std::runtime_error if failed to handle request
   */
  static bool isPortFree(std::string_view ip, in_port_t port, bool isTCP = true);

  /** 
  * @brief Find first available loopback address for given port.
  * @param port target port
  * @param isTCP whether to check TCP socket type, otherwise UDP is searched.
  * @retval found address if succeded
  * @retval empty string otherwise
  * @note Searches along 127.x.x.x/8 network. **Performs** sending ICMPv4 requests.
  */
  static std::string findFreeLoopbackAddress(in_port_t port, bool isTCP = true);

  /**
   * @brief Find first available private ip address for client in VPN tunnel.
   * @param gatewayCIDR gateway cidr. Assumed to be 1st address in network
   * @param cidrs array of busy address already given to client. Exclude it from searching.
   * @retval found address if succeded
   * @retval empty string otherwise
   * @note Searches along given gatewayCIDR network. **Not** performs ICMPv4 requests.
   */
  static std::string findFreeClientTunnelCIDR(std::string_view gatewayCIDR,
                                              const std::vector<std::string>& cidrs);

  /**
   * @brief Get gateway cidr. Assumes gateway address is 1st one in network.
   * @param net_cidr network cidr.
   * @retval gateway string if succeded
   * @retval empty string if net_cidr is invalid
   */
  static std::string getGatewayCIDR(std::string_view net_cidr);

private:
  /**
   * @brief Convert ip and prefix integer representation to string.
   * @param ip_host host endian ip address integer representation
   * @param prefix network prefix. Assumes to be in range [0; 32]
   * @retval ip string cidr representation.
   * @warning Various validations are **not** provided.
   */
  static std::string toCIDR4Impl(uint32_t ip_host, uint8_t prefix);

  /**
   * @brief Parse cidr to integer representation.
   * @param cidr host cidr
   * @retval std::tuple of ip address, network address and network prefix respectively in host endian
   * @retval std::nullopt if parsing failed
   */
  static std::optional<std::tuple<uint32_t, uint32_t, uint8_t>> parseCIDR4Impl(
      std::string_view cidr);
};

}  // namespace WgForge

#endif  // WGFORGE_INETUTILS_H
