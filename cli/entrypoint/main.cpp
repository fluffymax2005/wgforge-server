/*
 * main.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/di.hpp>
#include <CLI/CLI.hpp>
#include <iostream>

#include "boost_beast_http_sender.h"
#include "cli_app.h"
#include "data_processor.h"
#include "interface_api.h"
#include "interface_manager.h"
#include "interface_serializer.h"
#include "peer_api.h"
#include "peer_manager.h"
#include "peer_serializer.h"
#include "server_manager.h"

int main(int argc, char* argv[]) {
  auto cfg_mgr = std::make_shared<WgForge::ConfigManager>();

  using namespace boost;
  const auto injector = di::make_injector(
      // Bind instances
      di::bind<WgForge::ConfigManager>().to(cfg_mgr),

      // Bind global objects
      di::bind<std::ostream>().to(std::cout),

      // Bind interfaces
      di::bind<WgForge::IHttpSender>().to<WgForge::BoostBeastHttpSender>(),

      di::bind<WgForge::IPeerApi>().to<WgForge::PeerApi>(),
      di::bind<WgForge::IInterfaceApi>().to<WgForge::InterfaceApi>(),

      di::bind<WgForge::ISerializer<WgForge::Peer>>().to<WgForge::PeerSerializer>(),
      di::bind<WgForge::ISerializer<WgForge::InterfaceConfig>>().to<WgForge::InterfaceSerializer>(),
      di::bind<WgForge::IDataProcessor>().to<WgForge::DataProcessor>(),

      di::bind<WgForge::IPeerManager>().to<WgForge::PeerManager>(),
      di::bind<WgForge::IInterfaceManager>().to<WgForge::InterfaceManager>(),
      di::bind<WgForge::IServerManager>().to<WgForge::ServerManager>());

  auto app = injector.create<std::unique_ptr<WgForge::CliApp>>();
  app->run(argc, argv);
}
