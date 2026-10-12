/*
 * wireguard_interface_service.h
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

#ifndef __WGFORGE_WIREGUARD_INTERFACE_SERVICE_H__
#define __WGFORGE_WIREGUARD_INTERFACE_SERVICE_H__

#include <memory>

#include "config_manager.h"
#include "iinterfaceservice.h"
#include "ilogger.h"

namespace WgForge {

class WireguardInterfaceService : public IInterfaceService {
public:
  WireguardInterfaceService(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<ConfigManager> config_mgr,
                            std::shared_ptr<WgInterface> iface);

  virtual InterfaceInfoResponse getInfo() const override;
  virtual void updateConfig(const InterfaceUpdateRequest& req) override;

private:
  std::shared_ptr<ILogger> logger_;
  std::shared_ptr<ConfigManager> config_mgr_;
  std::shared_ptr<WgInterface> iface_;
};

}  // namespace WgForge


#endif  // __WGFORGE_WIREGUARD_INTERFACE_SERVICE_H__
