/*
 * server_manager.h
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_SERVER_MANAGER_H
#define WGFORGE_SERVER_MANAGER_H

#include <config_manager.hpp>
#include <memory>

#include "iservermanager.h"

namespace WgForge {

class ServerManager : public IServerManager {
public:
  ServerManager(std::shared_ptr<ConfigManager> cfg_mgr);

  virtual ServerConfig getConfig(const std::string& iface) const override;
  virtual void updateConfig(const UpdateServerRequest& req) override;

private:
  std::shared_ptr<ConfigManager> cfg_mgr_;

  bool ensureConfigOpenedImpl(const std::string& iface, bool readonly = true) const;
};

}  // namespace WgForge

#endif  // WGFORGE_SERVER_MANAGER_H
