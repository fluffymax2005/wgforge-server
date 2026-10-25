/*
 * peer_api.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_PEER_API_H
#define WGFORGE_PEER_API_H

#include <boost/url.hpp>
#include <config_manager.hpp>
#include <memory>

#include "ihttpsender.h"
#include "ipeerapi.h"
#include "iserializer.h"

namespace WgForge {

namespace urls = boost::urls;

class PeerApi : public IPeerApi {
public:
  PeerApi(std::shared_ptr<IHttpSender> sender, std::shared_ptr<ISerializer<Peer>> serializer,
          std::shared_ptr<ConfigManager> cfg_mgr);

  virtual std::optional<Peer> get(const GetPeerRequest& req) const override;
  virtual std::optional<std::vector<Peer>> getAll(const GetAllPeerRequest& req) const override;
  virtual CreatePeerResponse create(const CreatePeerRequest& req) override;
  virtual void update(const UpdatePeerRequest& req) override;
  virtual void softDelete(const DeletePeerRequest& req) override;

  virtual RotatePeerPublicKeyResponse rotatePublicKey(
      const RotatePeerPublicKeyRequest& req) override;
  virtual std::string getConfig(const GetPeerConfigRequest& req) const override;

private:
  std::shared_ptr<IHttpSender> sender_;
  std::shared_ptr<ISerializer<Peer>> serializer_;
  std::shared_ptr<ConfigManager> cfg_mgr_;

  bool ensureConfigOpenedImpl(const std::string& iface) const;
  urls::url getEncodedUrlImpl(std::optional<std::string> endpoint) const;
};

}  // namespace WgForge


#endif  // WGFORGE_PEER_API_H
