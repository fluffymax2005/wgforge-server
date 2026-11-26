/*
 * wgforgeserver.h
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

#ifndef WGFORGE_WGFORGESERVER_H
#define WGFORGE_WGFORGESERVER_H

#include <boost/asio.hpp>
#include <string>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace WgForge {

class WgForgeServer {
public:
  WgForgeServer(const WgForgeServer&) = delete;
  WgForgeServer& operator=(const WgForgeServer&) = delete;

  explicit WgForgeServer(const std::string& address, const std::string& port);

  void setHandler(RequestHandler&& handler) noexcept;

  boost::asio::io_context& getContext() noexcept;
  void run();

private:
  void do_accept();
  void do_await_stop();

  boost::asio::io_context io_context_;
  boost::asio::signal_set signals_;
  boost::asio::ip::tcp::acceptor acceptor_;
  ConnectionManager con_manager_;
  RequestHandler req_handler_;
};

}  // namespace WgForge

#endif  // WGFORGE_WGFORGESERVER_H
