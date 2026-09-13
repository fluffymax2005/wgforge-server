#ifndef __WGFORGE_PEER_RECORD_H__
#define __WGFORGE_PEER_RECORD_H__

#include <boost/json.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace WgForge {

namespace json = boost::json;

struct PeerRecord {
  int64_t id;
  std::string public_key;
  std::optional<std::string> preshared_key;
  std::optional<std::string> private_key;
  uint16_t persistent_keepalive;
  std::optional<std::string> description;
  std::string created_at;
  std::optional<std::string> updated_at;
  std::optional<std::string> deleted_at;
};

struct PeerConfig {
  PeerRecord record;
  std::vector<std::string> allowed_ips;

  operator json::object() const &;
  operator json::object() &&;
};

} // namespace WgForge

#endif // __WGFORGE_PEER_RECORD_H__
