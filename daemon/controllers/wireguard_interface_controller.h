/*
 * wireguard_interface_controller.h
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

#ifndef WGFORGE_WIREGUARD_INTERFACE_CONTROLLER_H
#define WGFORGE_WIREGUARD_INTERFACE_CONTROLLER_H

#include <config_manager.hpp>
#include <memory>

#include "defines.h"
#include "iexceptionhandle.h"
#include "iinterfaceservice.h"
#include "ilogger.h"

namespace WgForge {

class WireguardInterfaceController : IExceptionHandle {
public:
  WireguardInterfaceController(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<ConfigManager> config_mgr,
                               std::shared_ptr<IInterfaceService> iface_service);

  void getInfo(RequestConstRef req, ResponseRef res);
  void updateConfig(RequestConstRef req, ResponseRef res);

private:
  std::shared_ptr<ILogger> logger_;
  std::shared_ptr<ConfigManager> config_mgr_;
  std::shared_ptr<IInterfaceService> iface_service_;

  virtual void handleException(std::exception_ptr ep, RequestConstRef req,
                               ResponseRef res) override;
};

}  // namespace WgForge


#endif  // WGFORGE_WIREGUARD_INTERFACE_CONTROLLER_H
