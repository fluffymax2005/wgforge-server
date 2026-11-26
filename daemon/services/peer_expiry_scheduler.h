/*
 * peer_expiry_scheduler.h
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

#ifndef WGFORGE_PEER_EXPIRY_SCHEDULER_H
#define WGFORGE_PEER_EXPIRY_SCHEDULER_H

#include <boost/asio.hpp>
#include <chrono>
#include <memory>

#include "ilogger.h"
#include "ipeerservice.h"

namespace WgForge {

namespace asio = boost::asio;

class PeerExpiryScheduler : public std::enable_shared_from_this<PeerExpiryScheduler> {
public:
  PeerExpiryScheduler(asio::io_context& ioc, std::shared_ptr<IPeerServiceAsync> peer_service,
                      std::shared_ptr<ILogger> logger);

  void setUpdateTime(std::chrono::seconds time);
  void start();

private:
  void scheduleNextCheck();
  void onTimerFired();

  asio::steady_timer timer_;
  std::shared_ptr<IPeerServiceAsync> peer_service_;
  std::shared_ptr<ILogger> logger_;
  std::chrono::seconds update_time_;
};

}  // namespace WgForge

#endif
