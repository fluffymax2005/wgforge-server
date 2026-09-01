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
};

} // namespace WgForge

#endif
