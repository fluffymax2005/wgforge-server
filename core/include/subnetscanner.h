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

// IPv4 only
class SubnetScanner {
public:
  SubnetScanner() = delete;

  static std::optional<bool> isSubnetFree(std::string_view cidr, float timeout_seconds = 1);
  static std::string findFreeSubnet(uint8_t suffix, float timeout_seconds = 1);
  static std::string getGatewayCIDR(std::string_view net_cidr);

private:
  static unsigned short checksum(void* b, int len);
  static void ping_host(int id, std::string_view ipv4_address, std::atomic<bool>& network_has_host,
                        float timeout_seconds = 1);
  static std::optional<std::string> scanSubnetClass(uint32_t range_start, uint32_t range_end,
                                                    uint32_t step, uint8_t suffix,
                                                    float timeout_seconds = 1);
};

#endif // __WGFORGE_SUBNETSCANNER_H__
