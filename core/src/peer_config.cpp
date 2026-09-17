#include "peer_config.h"

namespace WgForge {

PeerConfig::operator json::object() const & {
  json::object o;
  o["id"] = record.id;
  o["public_key"] = record.public_key;
  if (record.preshared_key)
    o["preshared_key"] = *record.preshared_key;
  if (record.private_key)
    o["private_key"] = *record.private_key;
  o["CIDR"] = record.cidr;
  o["persistent_keepalive"] = record.persistent_keepalive;
  if (record.description)
    o["description"] = *record.description;
  o["created_at"] = record.created_at;
  if (record.updated_at)
    o["updated_at"] = *record.updated_at;
  if (record.deleted_at)
    o["deleted_at"] = *record.deleted_at;

  json::array arr;
  arr.reserve(allowed_ips.size());
  for (const auto &ip : allowed_ips)
    arr.emplace_back(ip);
  o["allowed_ips"] = std::move(arr);

  return o;
}

PeerConfig::operator json::object() && {
  json::object o;
  o["id"] = record.id;
  o["public_key"] = std::move(record.public_key);
  if (record.preshared_key)
    o["preshared_key"] = std::move(*record.preshared_key);
  if (record.private_key)
    o["private_key"] = std::move(*record.private_key);
  o["CIDR"] = std::move(record.cidr);
  o["persistent_keepalive"] = record.persistent_keepalive;
  if (record.description)
    o["description"] = std::move(*record.description);
  o["created_at"] = std::move(record.created_at);
  if (record.updated_at)
    o["updated_at"] = std::move(*record.updated_at);
  if (record.deleted_at)
    o["deleted_at"] = std::move(*record.deleted_at);

  json::array arr;
  arr.reserve(allowed_ips.size());
  for (const auto &ip : allowed_ips)
    arr.emplace_back(ip);
  o["allowed_ips"] = std::move(arr);

  return o;
}

} // namespace WgForge
