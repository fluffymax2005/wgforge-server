#include "subnetscanner.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <netinet/in.h>
#include <thread>
#include <vector>

std::optional<bool> SubnetScanner::isSubnetFree(std::string_view cidr,
                                                float timeout_seconds) {
  // Parsing
  const auto slash_pos = cidr.find_first_of('/');
  if (slash_pos == std::string_view::npos ||
      slash_pos != cidr.find_last_of('/'))
    return std::nullopt;

  int32_t suffix;
  auto [ptr, ec] = std::from_chars(cidr.data() + slash_pos + 1,
                                   cidr.data() + cidr.size(), suffix);
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
  uint32_t *net_ptr = reinterpret_cast<uint32_t *>(binary_addr.data());
  *net_ptr = htonl(ntohl(*net_ptr) & bit_mask);

  // Broadcast addr
  const uint32_t broadcast = ntohl(*net_ptr) | ~bit_mask;

  // Start and end addr
  const uint32_t start_ip = ntohl(*net_ptr) + 1;
  const uint32_t end_ip = broadcast - 1;

  if (start_ip > end_ip)
    return true;

  // Threads to execute ICMP request
  uint16_t num_threads = std::thread::hardware_concurrency();
  num_threads = std::max(1, num_threads - 1);

  // Threads pool
  std::atomic<bool> network_has_host{false};
  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  // Hosts counter for threads
  uint32_t current_ip = start_ip;
  uint32_t total_hosts = end_ip - start_ip + 1;

  // Ping range of hosts per thread
  auto ping_range = [&](uint32_t range_start, uint32_t range_end) {
    for (uint32_t ip = range_start; ip <= range_end && !network_has_host.load();
         ip++) {
      // Convert IP to str
      in_addr addr_struct;
      addr_struct.s_addr = htonl(ip);
      char ip_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &addr_struct, ip_str, sizeof(ip_str));

      // Send ICMP request
      ping_host(ip, ip_str, network_has_host, timeout_seconds);
    }
  };

  const uint32_t hosts_per_thread = total_hosts / num_threads;
  uint32_t remaining_hosts = total_hosts % num_threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    const uint32_t range_start = current_ip;
    uint32_t range_end = current_ip + hosts_per_thread - 1;

    if (i == num_threads - 1)
      range_end += remaining_hosts;
    if (range_start <= range_end)
      threads.emplace_back(ping_range, range_start, range_end);

    current_ip = range_end + 1;
  }

  std::for_each(threads.begin(), threads.end(), [](auto &t) {
    if (t.joinable())
      t.join();
  });

  return !network_has_host.load();
}

std::string SubnetScanner::findFreeSubnet(uint8_t suffix,
                                          float timeout_seconds) {
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
  auto scan_opt =
      scanSubnetClass(0x0A000000, 0x0AFFFFFF, step, suffix, timeout_seconds);
  if (scan_opt.has_value())
    return scan_opt.value();

  // Class B aka 172.16.0.0 - 172.31.255.255
  scan_opt =
      scanSubnetClass(0xAC100000, 0xAC1FFFFF, step, suffix, timeout_seconds);
  if (scan_opt.has_value())
    return scan_opt.value();

  // Class C aka 192.168.0.0 - 192.168.255.255
  scan_opt =
      scanSubnetClass(0xC0A80000, 0xC0A8FFFF, step, suffix, timeout_seconds);
  if (scan_opt.has_value())
    return scan_opt.value();
  return "";
}

std::optional<std::string>
SubnetScanner::scanSubnetClass(uint32_t range_start, uint32_t range_end,
                               uint32_t step, uint8_t suffix,
                               float timeout_seconds) {
  // Check subnet
  auto check_subnet = [&](uint32_t network_addr) -> bool {
    // Convert addr to str
    in_addr addr;
    addr.s_addr = htonl(network_addr);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

    char cidr_str[INET_ADDRSTRLEN + 1 +
                  2]; // INET_ADDRSTRLEN + '/' + "32" at maximum length
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

unsigned short SubnetScanner::checksum(void *b, int len) {
  unsigned short *buf = static_cast<unsigned short *>(b);
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;
  if (len == 1)
    sum += *(unsigned char *)buf;
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

void SubnetScanner::ping_host(int id, std::string_view ipv4_address,
                              std::atomic<bool> &network_has_host,
                              float timeout_seconds) {
  if (network_has_host.load()) {
    return;
  }

  // Create UDP ICMP socket
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (sock < 0) {
    return;
  }

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
  int bytes_sent =
      sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0,
             reinterpret_cast<sockaddr *>(&dest_addr), sizeof(dest_addr));
  if (bytes_sent <= 0) {
    close(sock);
    return;
  }

  // Get response with timeout
  sockaddr_in src_addr;
  socklen_t src_len = sizeof(src_addr);
  unsigned char recv_buffer[1024];

  timeval timeout;
  timeout.tv_sec = static_cast<time_t>(timeout_seconds);
  timeout.tv_usec =
      static_cast<suseconds_t>((timeout_seconds - timeout.tv_sec) * 1000000.);

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  int bytes_recv = recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,
                            reinterpret_cast<sockaddr *>(&src_addr), &src_len);

  if (bytes_recv < 0) {
    close(sock);
    return;
  }

  // Parse response
  iphdr *ip_hdr = reinterpret_cast<iphdr *>(recv_buffer);
  int ip_header_len = ip_hdr->ihl * 4;
  icmphdr *icmp_reply =
      reinterpret_cast<icmphdr *>(recv_buffer + ip_header_len);

  if (icmp_reply->type == ICMP_ECHOREPLY) {
    network_has_host.store(true);
  }

  close(sock);
}
