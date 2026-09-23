//
// Connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/*
 * connection.cpp
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

#include "connection.h"

#include <boost/asio/error.hpp>
#include <utility>

#include "connection_manager.h"
#include "request_handler.h"

namespace WgForge {

Connection::Connection(boost::asio::ip::tcp::socket socket, ConnectionManager& manager,
                       RequestHandler& handler)
    : socket_(std::move(socket)), con_manager_(manager), req_handler_(handler) {
}

void Connection::start() {
  do_read();
}

void Connection::stop() {
  socket_.close();
}

void Connection::do_read() {
  auto self(shared_from_this());

  beast::http::async_read(socket_, buffer_, request_,
                          [this, self](beast::error_code ec, std::size_t) {
	                        if (!ec) {
	                          req_handler_.handle_request(request_, response_);
	                          do_write();
	                        } else if (ec != boost::asio::error::operation_aborted ||
							           ec == boost::asio::error::operation_aborted) {
	                          con_manager_.stop(shared_from_this());
	                        }
                          });
}

void Connection::do_write() {
  auto self(shared_from_this());

  beast::http::async_write(socket_, response_, [this, self](beast::error_code ec, std::size_t) {
	if (ec != boost::asio::error::operation_aborted) {
	  con_manager_.stop(shared_from_this());
	}
  });
}

} // namespace WgForge
