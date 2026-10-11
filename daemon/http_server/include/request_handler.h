//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/*
 * request_handler.h
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

#ifndef __WGFORGE_REQUEST_HANDLER_H__
#define __WGFORGE_REQUEST_HANDLER_H__

#include <boost/beast/http.hpp>
#include <boost/beast/http/verb.hpp>
#include <string>
#include <unordered_map>

#include "defines.h"

namespace beast = boost::beast;

namespace WgForge {

struct RouteKey {
  http::verb method;
  std::string path;

  bool operator==(const RouteKey& other) const {
	return method == other.method && path == other.path;
  }
};

struct RouteKeyHash {
  size_t operator()(const RouteKey& key) const {
	return std::hash<int>()(static_cast<int>(key.method)) ^ std::hash<std::string_view>()(key.path);
  }
};

/// The common handler for all incoming requests.
class RequestHandler {
public:
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  using HandlerFunc = std::function<void(const http::request<beast::http::string_body>&,
                                         http::response<beast::http::string_body>&)>;

  RequestHandler() = default;

  RequestHandler(RequestHandler&&) = default;
  RequestHandler& operator=(RequestHandler&&) = default;

  void add_route(http::verb method, const std::string& uri, HandlerFunc handler);
  void handle_request(const http::request<http::string_body>& req,
                      http::response<http::string_body>& res);

  template<typename Controller, typename Method>
  void bind_route(http::verb method, const std::string& path,
                  std::shared_ptr<Controller> controller, Method method_ptr);

private:
  std::unordered_map<RouteKey, HandlerFunc, RouteKeyHash> routes_;
};

template<typename Controller, typename Method>
void RequestHandler::bind_route(http::verb method, const std::string& path,
                                std::shared_ptr<Controller> controller, Method method_ptr) {
  add_route(method, path, [controller, method_ptr](Request req, Response res) {
	(controller.get()->*method_ptr)(req, res);
  });
}

}  // namespace WgForge

#endif  // __WGFORGE_REQUEST_HANDLER_H__
