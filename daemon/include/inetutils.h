#ifndef __WGFORGE_INETUTILS_H__
#define __WGFORGE_INETUTILS_H__

#include "defines.h"

namespace WgForge {

class InetUtils {
public:
  // Assign IP address to Wireguard interface
  static void assignIpAddress(const InterfaceConfig &config);

  // Check whether port is free. Default is TCP, UDP otherwise checked
  static bool isPortFree(std::string_view ip, in_port_t port,
                         bool isTCP = true);

  // Find first available lo address for given port
  static std::string findFreeLoopbackAddress(in_port_t port, bool isTCP = true);

  static std::string
  findFreeClientTunnelCIDR(std::string_view gatewayCIDR,
                           const std::vector<std::string_view> cidrs);

private:
  static std::string toCIDR4Impl(uint32_t ip_host, uint8_t prefix);
  static std::optional<std::pair<uint32_t, uint8_t>>
  parseCIDR4Impl(std::string_view cidr);
};

} // namespace WgForge

#endif
