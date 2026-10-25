/*
 * connection.h
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

#ifndef WGFORGE_CONNECTION_H
#define WGFORGE_CONNECTION_H

#include <array>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>

#include "defines.h"
#include "request_handler.h"

namespace WgForge {

class ConnectionManager;

/// Represents a single Connection from a client.
class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  /// Construct a Connection with the given socket.
  explicit Connection(boost::asio::ip::tcp::socket socket, ConnectionManager& manager,
                      RequestHandler& handler);

  /// Start the first asynchronous operation for the Connection.
  void start();

  /// Stop all asynchronous operations associated with the Connection.
  void stop();

private:
  /// Perform an asynchronous read operation.
  void do_read();

  /// Perform an asynchronous write operation.
  void do_write();

  /// Socket for the Connection.
  boost::asio::ip::tcp::socket socket_;

  /// The manager for this Connection.
  ConnectionManager& con_manager_;

  /// The handler used to process the incoming request.
  RequestHandler& req_handler_;

  beast::flat_buffer buffer_;

  /// The incoming request.
  http::request<beast::http::string_body> request_;
  http::response<beast::http::string_body> response_;
};

using connection_ptr = std::shared_ptr<Connection>;

} // namespace WgForge

#endif // WGFORGE_CONNECTION_H
