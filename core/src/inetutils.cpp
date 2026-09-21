#include "inetutils.h"

#include <netlink/netlink.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>

#include <format>
#include <unordered_set>

namespace WgForge {

void InetUtils::assignIpAddress(const InterfaceConfig& config) {
  std::string_view addr_view{config.addr};
  const auto slash_pos = addr_view.find_first_of('/');
  if (slash_pos != addr_view.find_last_of('/'))
	throw std::invalid_argument("Invalid interface address CIDR provided");

  in_addr ipv4_addr;
  char ip_text_addr[sizeof(config.addr) - 5]{}; // not include [], :, port number

  std::strncpy(ip_text_addr, config.addr,
               slash_pos); // copy address

  if (inet_pton(AF_INET, ip_text_addr, &ipv4_addr) != 1)
	throw std::invalid_argument("Invalid interface IP address provided");

  nl_sock* sock = nl_socket_alloc();
  if (sock == nullptr)
	throw std::runtime_error("Failed to allocate nl socket to configure interface");

  auto ec = nl_connect(sock, NETLINK_ROUTE);
  if (ec < 0) {
	nl_socket_free(sock);
	throw std::runtime_error("Failed to connect nl socket to kernel");
  }

  nl_cache* cache;
  if ((ec = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
	nl_close(sock);
	nl_socket_free(sock);
	throw std::runtime_error("Failed to allocate rtnl link cache");
  }

  rtnl_link* link = rtnl_link_get_by_name(cache, config.name);
  if (link == nullptr) {
	nl_cache_free(cache);
	nl_close(sock);
	nl_socket_free(sock);
	throw std::runtime_error("Failed to get rtnl link");
  }

  rtnl_addr* addr = rtnl_addr_alloc();
  if (addr == nullptr) {
	rtnl_link_put(link);
	nl_cache_free(cache);
	nl_close(sock);
	nl_socket_free(sock);
	throw std::runtime_error("Failed to allocate rntl address");
  }

  rtnl_addr_set_ifindex(addr, rtnl_link_get_ifindex(link));
  rtnl_addr_set_family(addr, AF_INET);

  nl_addr* local_addr = nl_addr_build(AF_INET, &ipv4_addr, sizeof(ipv4_addr));
  if (local_addr == nullptr) {
	rtnl_addr_put(addr);
	rtnl_link_put(link);
	nl_cache_free(cache);
	nl_close(sock);
	nl_socket_free(sock);
	throw std::runtime_error("Failed to build network address");
  }

  rtnl_addr_set_local(addr, local_addr);

  int32_t prefix_len{};
  try {
	prefix_len = std::stoi(addr_view.data() + slash_pos + 1);
  } catch (...) {
	nl_addr_put(local_addr);
	rtnl_addr_put(addr);
	rtnl_link_put(link);
	nl_cache_free(cache);
	nl_close(sock);
	nl_socket_free(sock);
	throw std::runtime_error("Failed to convert prefix to int");
  }
  rtnl_addr_set_prefixlen(addr, prefix_len);

  ec = rtnl_addr_add(sock, addr, 0);

  nl_addr_put(local_addr);
  rtnl_addr_put(addr);
  rtnl_link_put(link);
  nl_cache_free(cache);
  nl_close(sock);
  nl_socket_free(sock);

  if (ec < 0)
	throw std::runtime_error("Failed to add interface address");
}

bool InetUtils::isPortFree(std::string_view ip, in_port_t port, bool isTCP) {
  auto sock = socket(AF_INET, isTCP ? SOCK_STREAM : SOCK_DGRAM, 0);
  if (sock < 0)
	throw std::runtime_error("Failed to open socket to check port availability");

  int opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	throw std::runtime_error("Failed to set socket option SO_REUSEADDR");

  // Create socket address to check
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.data(), &addr.sin_addr);

  // Try bind address to socket
  auto ec = bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  auto bind_errno = errno;
  close(sock);

  // Check error codes
  if (ec == 0)
	return true;
  if (bind_errno == EADDRINUSE)
	return false;

  // Other errors
  throw std::runtime_error(std::format("Failed to check port availability. Error code: {}", errno));
}

std::string InetUtils::findFreeLoopbackAddress(in_port_t port, bool isTCP) {
  char buf[INET_ADDRSTRLEN]{};
  for (uint16_t o2 = 0; o2 < 256; o2++) {
	for (uint16_t o3 = 0; o3 < 256; o3++) {
	  for (uint16_t o4 = 1; o4 < 256; o4++) {
		std::snprintf(buf, sizeof(buf), "127.%i.%i.%i", o2, o3, o4);
		try {
		  if (isPortFree(buf, port, isTCP))
			return buf;
		} catch (...) {
		}
	  }
	}
  }

  return "";
}

std::string InetUtils::getGatewayCIDR(std::string_view net_cidr) {
  const auto slash_pos = net_cidr.find('/');
  if (slash_pos == std::string_view::npos || slash_pos != net_cidr.find_first_of('/'))
	return "";

  std::string_view addr_sv = net_cidr.substr(0, slash_pos);
  std::string_view prefix_sv = net_cidr.substr(slash_pos + 1);

  // Parse prefix
  int prefix = 0;
  auto [ptr, ec] = std::from_chars(prefix_sv.data(), prefix_sv.data() + prefix_sv.size(), prefix);
  if (ec != std::errc() || ptr != prefix_sv.data() + prefix_sv.size())
	return "";
  if (prefix < 1 || prefix > 31)
	return "";

  // Parse address
  std::string addr_str(addr_sv);
  in_addr in;
  if (inet_pton(AF_INET, addr_str.c_str(), &in) != 1)
	return "";

  uint32_t ip = ntohl(in.s_addr);

  // 4. Address and net mask
  uint32_t mask = 0xFFFFFFFFu << (32 - prefix);
  uint32_t network = ip & mask;
  uint32_t gateway = network + 1;

  in_addr gw_in{};
  gw_in.s_addr = htonl(gateway);

  char buf[INET_ADDRSTRLEN];
  if (!inet_ntop(AF_INET, &gw_in, buf, sizeof(buf)))
	return "";

  return std::format("{}/{}", buf, prefix);
}

std::string InetUtils::findFreeClientTunnelCIDR(std::string_view gatewayCIDR,
                                                const std::vector<std::string>& cidrs) {
  auto parsed = parseCIDR4Impl(gatewayCIDR);
  if (!parsed)
	return "";

  auto [ip, network, prefix] = *parsed;
  const uint32_t host_count =
      (1u << (32 - prefix)) - 3; // subnet, getway and broadcast addressed are not included
  const uint32_t first_host = network + 1;
  const uint32_t last_host = first_host + host_count;

  std::unordered_set<uint32_t> used;
  for (const auto& c : cidrs) {
	auto p = parseCIDR4Impl(c);
	if (!p)
	  continue;
	used.insert(std::get<0>(*p));
  }
  used.insert(first_host);

  for (uint32_t ip = first_host + 1; ip < last_host; ip++) {
	if (used.find(ip) == used.end())
	  return toCIDR4Impl(ip, prefix);
  }

  return "";
}

std::string InetUtils::toCIDR4Impl(uint32_t ip_host, uint8_t prefix) {
  in_addr in;
  in.s_addr = ntohl(ip_host);
  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &in, buf, sizeof(buf));
  return std::format("{}/{}", buf, prefix);
}

std::optional<std::tuple<uint32_t, uint32_t, uint8_t>> InetUtils::parseCIDR4Impl(
    std::string_view cidr) {
  const auto slash_pos = cidr.find_first_of('/');
  if (slash_pos == std::string_view::npos || slash_pos != cidr.find_last_of('/'))
	return std::nullopt;

  std::string addr(cidr.substr(0, slash_pos));
  int prefix;
  try {
	prefix = std::stoi(std::string(cidr.substr(slash_pos + 1)));
  } catch (...) {
	return std::nullopt;
  }

  if (prefix < 0 || prefix > 32)
	return std::nullopt;

  in_addr in;
  if (inet_pton(AF_INET, addr.c_str(), &in) != 1)
	return std::nullopt;

  uint32_t ip = ntohl(in.s_addr);
  uint32_t mask = prefix == 0 ? 0 : (0xFFFFFFFFu << (32 - prefix));
  uint32_t network = ip & mask;

  return std::make_tuple(ip, network, static_cast<uint8_t>(prefix));
}

} // namespace WgForge
