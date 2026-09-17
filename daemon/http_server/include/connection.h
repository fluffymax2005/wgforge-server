//
// Connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __WGFORGE_CONNECTION_H__
#define __WGFORGE_CONNECTION_H__

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

#endif // __WGFORGE_CONNECTION_H__
