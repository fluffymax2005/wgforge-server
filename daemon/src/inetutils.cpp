#include "inetutils.h"

#include <netlink/netlink.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>

namespace WgForge {

void InetUtils::assignIpAddress(const InterfaceConfig &config) {
  std::string_view addr_view{config.addr};
  const auto slash_pos = addr_view.find_first_of('/');
  if (slash_pos != addr_view.find_last_of('/'))
    throw std::invalid_argument("Invalid interface address CIDR provided");

  in_addr ipv4_addr;
  char
      ip_text_addr[sizeof(config.addr) - 5]{}; // not include [], :, port number

  std::strncpy(ip_text_addr, config.addr,
               slash_pos); // copy address

  if (inet_pton(AF_INET, ip_text_addr, &ipv4_addr) != 1)
    throw std::invalid_argument("Invalid interface IP address provided");

  nl_sock *sock = nl_socket_alloc();
  if (sock == nullptr)
    throw std::runtime_error(
        "Failed to allocate nl socket to configure interface");

  auto ec = nl_connect(sock, NETLINK_ROUTE);
  if (ec < 0) {
    nl_socket_free(sock);
    throw std::runtime_error("Failed to connect nl socket to kernel");
  }

  nl_cache *cache;
  if ((ec = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to allocate rtnl link cache");
  }

  rtnl_link *link = rtnl_link_get_by_name(cache, config.name);
  if (link == nullptr) {
    nl_cache_free(cache);
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to get rtnl link");
  }

  rtnl_addr *addr = rtnl_addr_alloc();
  if (addr == nullptr) {
    rtnl_link_put(link);
    nl_cache_free(cache);
    nl_close(sock);
    nl_socket_free(sock);
    throw std::runtime_error("Failed to allocate rntl address");
  }

  rtnl_addr_set_ifindex(addr, rtnl_link_get_ifindex(link));
  rtnl_addr_set_family(addr, AF_INET);

  nl_addr *local_addr = nl_addr_build(AF_INET, &ipv4_addr, sizeof(ipv4_addr));
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
    throw std::runtime_error(
        "Failed to open socket to check port availability");

  // Create socket address to check
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.data(), &addr.sin_addr);

  // Try bind address to socket
  auto ec = bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  close(sock);

  // Check error codes
  if (ec == 0)
    return true;
  if (errno == EADDRINUSE)
    return false;

  // Other errors
  throw std::runtime_error(
      std::format("Failed to check port availability. Error code: {}", errno));
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

} // namespace WgForge
