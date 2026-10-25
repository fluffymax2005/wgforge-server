/*
 * create_peer_response.hpp
 * wgforge-contracts — contracts for wgforge-server (C) and wgforge-cli  (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-contracts is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-contracts is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_CREATE_PEER_RESPONSE_HPP
#define WGFORGE_CREATE_PEER_RESPONSE_HPP

#include <boost/json.hpp>
#include <string>

namespace WgForge {

namespace json = boost::json;

struct CreatePeerResponse {
  std::string public_key;
  std::string private_key;
  std::string preshared_key;
  std::string tunnel_address;

  friend void tag_invoke(json::value_from_tag, json::value& jv, const CreatePeerResponse& res);
  friend CreatePeerResponse tag_invoke(json::value_to_tag<CreatePeerResponse>,
                                       const json::value& jv);
};

inline void tag_invoke(json::value_from_tag, json::value& jv, const CreatePeerResponse& res) {
  jv = json::object{
      {   "publicKey",     res.public_key},
      {  "privateKey",    res.private_key},
      {"presharedKey",  res.preshared_key},
      {        "cidr", res.tunnel_address}
  };
}

inline CreatePeerResponse tag_invoke(json::value_to_tag<CreatePeerResponse>,
                                     const json::value& jv) {
  const auto& obj = jv.as_object();

  CreatePeerResponse res;
  res.public_key = json::value_to<std::string>(obj.at("publicKey"));
  res.private_key = json::value_to<std::string>(obj.at("privateKey"));
  res.preshared_key = json::value_to<std::string>(obj.at("presharedKey"));
  res.tunnel_address = json::value_to<std::string>(obj.at("cidr"));

  return res;
}

}  // namespace WgForge


#endif  // WGFORGE_CREATE_PEER_RESPONSE_HPP
