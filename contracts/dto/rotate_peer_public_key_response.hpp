/*
 * rotate_peer_public_key_response.hpp
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

#ifndef WGFORGE_ROTATE_PEER_PUBLIC_KEY_RESPONSE_HPP
#define WGFORGE_ROTATE_PEER_PUBLIC_KEY_RESPONSE_HPP

#include <boost/json.hpp>
#include <string>

#include "create_peer_response.hpp"

namespace WgForge {

namespace json = boost::json;

struct RotatePeerPublicKeyResponse {
  std::string private_key;
  std::string public_key;

  void tag_invoke(json::value_from_tag, json::value& jv, const RotatePeerPublicKeyResponse& res);
  RotatePeerPublicKeyResponse tag_invoke(json::value_to_tag<RotatePeerPublicKeyResponse>,
                                         const json::value& jv);
};

inline void tag_invoke(json::value_from_tag, json::value& jv,
                       const RotatePeerPublicKeyResponse& res) {
  jv = json::object{
      { "publicKey",  res.public_key},
      {"privateKey", res.private_key}
  };
}

inline RotatePeerPublicKeyResponse tag_invoke(json::value_to_tag<RotatePeerPublicKeyResponse>,
                                              const json::value& jv) {
  const auto& obj = jv.as_object();

  RotatePeerPublicKeyResponse res;
  res.public_key = json::value_to<std::string>(obj.at("publicKey"));
  res.private_key = json::value_to<std::string>(obj.at("privateKey"));

  return res;
}

}  // namespace WgForge

#endif  // WGFORGE_ROTATE_PEER_PUBLIC_KEY_RESPONSE_HPP
