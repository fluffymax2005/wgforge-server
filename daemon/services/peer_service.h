/*
 * peer_service.h
 * wgforge-server — Backend to manage Wireguard
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_PEER_SERVICE_H
#define WGFORGE_PEER_SERVICE_H

#include <odb/database.hxx>

#include "config_manager.h"
#include "iconfiguration_section.h"
#include "ilogger.h"
#include "ipeerservice.h"
#include "peer_config_request.hpp"
#include "wgforge.h"

namespace WgForge {

class PeerService : public IPeerServiceAsync {
public:
  explicit PeerService(std::shared_ptr<odb::database> context, std::shared_ptr<WgInterface> iface,
                       std::shared_ptr<ConfigManager> config_mgr, std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IConfiguration> configuration);

  virtual std::future<std::optional<Peer>> getPeerAsync(const Key& key) const override;
  virtual std::future<std::optional<Peer>> getPeerByPublicKey(std::string_view key) const override;
  virtual std::future<std::optional<std::vector<Peer>>> getAllAsync() const override;
  virtual std::future<CreatePeerResponse> createPeerAsync(const CreatePeerRequest& req) override;
  virtual std::future<void> softDeleteAsync(const Key& key) override;
  virtual std::future<void> hardDeleteAsync(const Key& key) override;

  virtual std::future<void> updateAsync(const UpdatePeerRequest& req) override;

  virtual std::future<std::optional<AllowedIP>> getAllowedIP(const Key& key) const override;
  virtual std::future<std::optional<std::vector<AllowedIP>>> getAllAllowedIPs(
      const Key& key) const override;
  virtual std::future<void> addAllowedIPAsync(const Key& key, std::string_view allowedip) override;
  virtual std::future<void> addAllowedIPsAsync(const Key& key,
                                               const std::vector<std::string>& allowedips) override;
  virtual std::future<void> softDeleteAllowedIPAsync(const Key& key,
                                                     std::string_view allowedip) override;
  virtual std::future<void> softDeleteAllowedIPsAsync(
      const Key& key, const std::vector<std::string>& allowedips) override;

  virtual std::future<RotatePeerPublicKeyResponse> rotatePublicKeyAsync(
      const RotatePeerPublicKeyRequest& req) override;
  virtual std::future<std::string> getPeerConfigAsync(const PeerConfigRequest& req) override;

private:
  std::shared_ptr<odb::database> context_;
  std::shared_ptr<WgInterface> iface_;
  std::shared_ptr<ConfigManager> config_mgr_;
  std::shared_ptr<ILogger> logger_;
  std::shared_ptr<IConfiguration> configuration_;

  std::string getCurrentUtcTimestamp();
};

}  // namespace WgForge

#endif  // WGFORGE_PEER_SERVICE_H
