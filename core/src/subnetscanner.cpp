/*
 * subnetscanner.cpp
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

#include "subnetscanner.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <thread>
#include <vector>

std::optional<bool> SubnetScanner::isSubnetFree(std::string_view cidr, float timeout_seconds) {
  // Parsing
  const auto slash_pos = cidr.find_first_of('/');
  if (slash_pos == std::string_view::npos || slash_pos != cidr.find_last_of('/'))
	return std::nullopt;

  int32_t suffix;
  auto [ptr, ec] = std::from_chars(cidr.data() + slash_pos + 1, cidr.data() + cidr.size(), suffix);
  if (ec != std::errc() || suffix < 1 || suffix > 31)
	return std::nullopt;

  std::array<char, INET_ADDRSTRLEN> addr{};
  cidr.remove_suffix(cidr.size() - slash_pos);
  std::strncpy(addr.data(), cidr.data(), cidr.size());

  std::array<uint8_t, 4> binary_addr;
  if (inet_pton(AF_INET, addr.data(), binary_addr.data()) != 1)
	return std::nullopt;

  // Subnet bit mask
  uint32_t bit_mask{};
  for (int8_t i = 31; i >= 0; i--, suffix--) {
	if (suffix == 0)
	  break;
	bit_mask = bit_mask | (1 << i);
  }

  // Subnet addr
  uint32_t* net_ptr = reinterpret_cast<uint32_t*>(binary_addr.data());
  *net_ptr = htonl(ntohl(*net_ptr) & bit_mask);

  // Broadcast addr
  const uint32_t broadcast = ntohl(*net_ptr) | ~bit_mask;

  // Start and end addr
  const uint32_t start_ip = ntohl(*net_ptr) + 1;
  const uint32_t end_ip = broadcast - 1;

  if (start_ip > end_ip)
	return true;
  return isSubnetFreeFast(start_ip, end_ip, timeout_seconds);
}

std::string SubnetScanner::findFreeSubnet(uint8_t suffix, float timeout_seconds) {
  if (suffix == 0 || suffix >= 32)
	return "";

  // Subnet bit mask
  uint32_t bit_mask{};
  uint8_t suffix_clone = suffix;
  for (int8_t i = 31; i >= 0; i--, suffix_clone--) {
	if (suffix_clone == 0)
	  break;
	bit_mask = bit_mask | (1 << i);
  }

  // Step: 2 ^ (32 - suffix)
  uint32_t step = 1 << (32 - suffix);

  // Class A aka 10.0.0.0 -> 10.255.255.255
  auto scan_opt = scanSubnetClass(0x0A000000, 0x0AFFFFFF, step, suffix, timeout_seconds);
  if (scan_opt.has_value())
	return scan_opt.value();

  // Class B aka 172.16.0.0 - 172.31.255.255
  scan_opt = scanSubnetClass(0xAC100000, 0xAC1FFFFF, step, suffix, timeout_seconds);
  if (scan_opt.has_value())
	return scan_opt.value();

  // Class C aka 192.168.0.0 - 192.168.255.255
  scan_opt = scanSubnetClass(0xC0A80000, 0xC0A8FFFF, step, suffix, timeout_seconds);
  if (scan_opt.has_value())
	return scan_opt.value();
  return "";
}

std::optional<std::string> SubnetScanner::scanSubnetClass(uint32_t range_start, uint32_t range_end,
                                                          uint32_t step, uint8_t suffix,
                                                          float timeout_seconds) {
  // Check subnet
  auto check_subnet = [&](uint32_t network_addr) -> bool {
	// Convert addr to str
	in_addr addr;
	addr.s_addr = htonl(network_addr);
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

	char cidr_str[INET_ADDRSTRLEN + 1 + 2];  // INET_ADDRSTRLEN + '/' + "32" at maximum length
	std::snprintf(cidr_str, sizeof(cidr_str), "%s/%d", ip_str, suffix);

	auto result = isSubnetFree(cidr_str, timeout_seconds);
	return result.has_value() && result.value();
  };

  for (uint32_t net = range_start; net <= range_end; net += step) {
	if (net + step - 1 > range_end)
	  break;

	// Found suitable subnet
	if (check_subnet(net)) {
	  // Convert addr to str
	  in_addr addr;
	  addr.s_addr = htonl(net);
	  char ip_str[INET_ADDRSTRLEN];
	  inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

	  char result[INET_ADDRSTRLEN + 3];
	  std::snprintf(result, sizeof(result), "%s/%d", ip_str, suffix);
	  return std::string(result);
	}
  }

  return std::nullopt;
}

bool SubnetScanner::isSubnetFreeFast(uint32_t start_ip, uint32_t end_ip, float timeout_seconds) {
  // 1. The only socket to scan
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0)
	throw std::runtime_error("Failed to create socket to send ICMP Echo Reqest");

  // 2. Change socket option to nonblocking
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  // 3. Quickly send ICMP Echo
  uint16_t pid_id = static_cast<uint16_t>(getpid() & 0xFFFF);
  uint16_t seq = 0;

  for (uint32_t ip = start_ip; ip <= end_ip; ++ip) {
	sockaddr_in dest_addr{};
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = htonl(ip);

	icmphdr icmp_req{};
	icmp_req.type = ICMP_ECHO;
	icmp_req.code = 0;
	icmp_req.un.echo.id = htons(pid_id);
	icmp_req.un.echo.sequence = htons(++seq);
	icmp_req.checksum = checksum(&icmp_req, sizeof(icmp_req));

	sendto(sock, &icmp_req, sizeof(icmp_req), 0, reinterpret_cast<sockaddr*>(&dest_addr),
	       sizeof(dest_addr));
  }

  // 4. Await response via poll() unless timeout occured
  auto start_time = std::chrono::steady_clock::now();
  int timeout_ms = static_cast<int>(timeout_seconds * 1000.0f);

  unsigned char recv_buf[1024];
  pollfd pfd{sock, POLLIN, 0};

  while (true) {
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
	                   std::chrono::steady_clock::now() - start_time)
	                   .count();

	int remaining = timeout_ms - static_cast<int>(elapsed);
	if (remaining <= 0) {
	  break;  // timeout
	}

	int ret = poll(&pfd, 1, remaining);
	if (ret <= 0) {
	  break;  // timeout or poll error
	}

	if (pfd.revents & POLLIN) {
	  sockaddr_in from{};
	  socklen_t from_len = sizeof(from);
	  ssize_t len = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
	                         reinterpret_cast<sockaddr*>(&from), &from_len);

	  if (len >= static_cast<ssize_t>(sizeof(iphdr) + sizeof(icmphdr))) {
		// Parse SOCK_RAW IP header
		iphdr* ip = reinterpret_cast<iphdr*>(recv_buf);
		int ip_len = ip->ihl * 4;

		if (len >= ip_len + static_cast<int>(sizeof(icmphdr))) {
		  icmphdr* icmp = reinterpret_cast<icmphdr*>(recv_buf + ip_len);

		  if (icmp->type == ICMP_ECHOREPLY && ntohs(icmp->un.echo.id) == pid_id) {
			uint32_t reply_ip = ntohl(from.sin_addr.s_addr);
			if (reply_ip >= start_ip && reply_ip <= end_ip) {
			  close(sock);
			  return false;  // subnet is not free
			}
		  }
		}
	  }
	}
  }

  close(sock);
  return true;  // subnet is free
}

unsigned short SubnetScanner::checksum(void* b, int len) {
  unsigned short* buf = static_cast<unsigned short*>(b);
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2)
	sum += *buf++;
  if (len == 1)
	sum += *(unsigned char*)buf;
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

bool SubnetScanner::ping_host(int id, std::string_view ipv4_address, float timeout_seconds) {
  // Create UDP ICMP socket
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (sock < 0)
	throw std::runtime_error("Failed to create socket to ping host");

  // Fill sockaddr with ipv4 address
  sockaddr_in dest_addr{};
  dest_addr.sin_family = AF_INET;
  inet_pton(AF_INET, ipv4_address.data(), &dest_addr.sin_addr);

  // Build ICMP request header
  icmphdr icmp_hdr{};
  icmp_hdr.type = ICMP_ECHO;
  icmp_hdr.un.echo.id = id;
  icmp_hdr.un.echo.sequence = 1;
  icmp_hdr.checksum = checksum(&icmp_hdr, sizeof(icmp_hdr));

  // Send request
  int bytes_sent = sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0,
                          reinterpret_cast<sockaddr*>(&dest_addr), sizeof(dest_addr));
  if (bytes_sent <= 0) {
	close(sock);
	throw std::runtime_error("Failed to send ICMP request");
  }

  // Get response with timeout
  sockaddr_in src_addr;
  socklen_t src_len = sizeof(src_addr);
  unsigned char recv_buffer[1024];

  timeval timeout;
  timeout.tv_sec = static_cast<time_t>(timeout_seconds);
  timeout.tv_usec = static_cast<suseconds_t>((timeout_seconds - timeout.tv_sec) * 1000000.);

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  int bytes_recv = recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,
                            reinterpret_cast<sockaddr*>(&src_addr), &src_len);

  if (bytes_recv < 0) {
	close(sock);
	return false;
  }


  // Parse response
  iphdr* ip_hdr = reinterpret_cast<iphdr*>(recv_buffer);
  int ip_header_len = ip_hdr->ihl * 4;
  icmphdr* icmp_reply = reinterpret_cast<icmphdr*>(recv_buffer + ip_header_len);

  close(sock);
  return icmp_reply->type == ICMP_ECHOREPLY;
}
