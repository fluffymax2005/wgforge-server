/*
 * wgforgeserver.cpp
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

#include "wgforgeserver.h"

#include <signal.h>

#include <random>
#include <utility>

namespace WgForge {

WgForgeServer::WgForgeServer(const std::string& address, const std::string& port)
    : io_context_(1)
    , signals_(io_context_)
    , acceptor_(io_context_)
    , con_manager_()
    , req_handler_() {
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif  // defined(SIGQUIT)

  do_await_stop();

  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(io_context_);
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, port).begin();
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  do_accept();
}

void WgForgeServer::run() {
  io_context_.run();
}

void WgForgeServer::do_accept() {
  acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
	// Check whether the server was stopped by a signal before this
	// completion handler had a chance to run.
	if (!acceptor_.is_open()) {
	  return;
	}

	if (!ec) {
	  con_manager_.start(
	      std::make_shared<Connection>(std::move(socket), con_manager_, req_handler_));
	}

	do_accept();
  });
}

void WgForgeServer::do_await_stop() {
  signals_.async_wait([this](boost::system::error_code /*ec*/, int /*signo*/) {
	// The server is stopped by cancelling all outstanding asynchronous
	// operations. Once all operations have finished the io_context::run()
	// call will exit.
	acceptor_.close();
	con_manager_.stop_all();
	io_context_.stop();
  });
}

void WgForgeServer::setHandler(RequestHandler&& handler) noexcept {
  req_handler_ = std::move(handler);
}

boost::asio::io_context& WgForgeServer::getContext() noexcept {
  return io_context_;
}

}  // namespace WgForge
