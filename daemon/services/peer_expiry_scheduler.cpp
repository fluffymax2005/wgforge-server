/*
 * peer_expiry_scheduler.cpp
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

#include "peer_expiry_scheduler.h"

#include <exception>
#include <format>

namespace WgForge {

PeerExpiryScheduler::PeerExpiryScheduler(asio::io_context& ioc,
                                         std::shared_ptr<IPeerServiceAsync> peer_service,
                                         std::shared_ptr<ILogger> logger)
    : timer_(ioc), update_time_(std::chrono::seconds(60)) {
  if (peer_service == nullptr)
	throw std::invalid_argument("Peer service is nullptr");
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");

  peer_service_ = std::move(peer_service);
  logger_ = std::move(logger);
}

void PeerExpiryScheduler::start() {
  scheduleNextCheck();
}

void PeerExpiryScheduler::setUpdateTime(std::chrono::seconds time) {
  if (time <= std::chrono::seconds(0))
	throw std::invalid_argument("time must be greater than zero");
  update_time_ = time;
}

void PeerExpiryScheduler::scheduleNextCheck() {
  timer_.expires_after(update_time_);

  timer_.async_wait([weak_self = weak_from_this()](const boost::system::error_code& ec) {
	if (ec == asio::error::operation_aborted)
	  return;  // server is requested to stop
	if (auto self = weak_self.lock())
	  self->onTimerFired();
  });
}

void PeerExpiryScheduler::onTimerFired() {
  try {
	logger_->info("Checking for expired temporary peers");
	peer_service_->revokeExpiredPeers().get();
  } catch (const std::exception& e) {
	logger_->error(std::format("Failed during peer cleanup. Reason: {}", e.what()));
  }

  scheduleNextCheck();
}

}  // namespace WgForge
