/*
 * subnetscanner.h
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

#ifndef __WGFORGE_SUBNETSCANNER_H__
#define __WGFORGE_SUBNETSCANNER_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <optional>
#include <string>

/**
 * @brief Provides utilities for scanning networks. Currently supports **only** IPv4.
 */
class SubnetScanner {
public:
  SubnetScanner() = delete;

  /**
   * @brief Check network occupation via ICMP.
   * @param cidr network CIDR to check
   * @param timeout_seconds time for timeout
   * @retval std::nullopt if cidr is invalid
   * @retval true if subnet is free
   * @retval false otherwise
   * @note Take all free CPU threads to send requests.
   */
  static std::optional<bool> isSubnetFree(std::string_view cidr, float timeout_seconds = 1);

  /**
   * @brief Perform scanning private network to found free one.
   * @param suffix network suffix
   * @param timeout_seconds time for timeout
   * @retval empty string if suffix is not in range [1; 32] or suitable network is not found
   * @retval cidr of found network otherwise
   * @note Scanning order: 10.x.x.x/8 then 172.16.x.x/16 - 172.31.x.x/16 then 192.168.x.x/24
   */
  static std::string findFreeSubnet(uint8_t suffix, float timeout_seconds = 1);

  /**
   * @brief Extract gateway cidr from network CIDR. Assumes gateway takes 1st address.
   * @param net_cidr network CIDR
   * @retval empty string if net_cidr is invalid or logic violation occures. For example, 192.168.1.0/32 has no gateway
   * @retval found cidr
   */
  static std::string getGatewayCIDR(std::string_view net_cidr);

private:
  /**
   * @brief Calculate checksum for ICMP request.
   * @param b body memory buffer
   * @param len length of buffer
   * @retval calculated checksum
   */
  static unsigned short checksum(void* b, int len);

  /**
   * @brief Send ICMP echo request.
   * @param id random identificator for request
   * @param ipv4_address address to send request to
   * @param network_has_host bool flag which is set to true if host replied
   * @param timeout_seconds time for timeout
   */
  static void ping_host(int id, std::string_view ipv4_address, std::atomic<bool>& network_has_host,
                        float timeout_seconds = 1);

  /**
   * @brief Scan A, B and C private network classes
   * @param range_start binary start address representation
   * @param range_end binary end address representation
   * @param step step to perform scanning to
   * @param suffix network suffix
   * @param timeout_seconds time for timeout
   * @retval std::nullopt if subnet is not free 
   * @retval string CIDR representation of free subnet
   */
  static std::optional<std::string> scanSubnetClass(uint32_t range_start, uint32_t range_end,
                                                    uint32_t step, uint8_t suffix,
                                                    float timeout_seconds = 1);
};

#endif // __WGFORGE_SUBNETSCANNER_H__
