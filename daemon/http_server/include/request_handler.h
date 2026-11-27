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

#ifndef WGFORGE_REQUEST_HANDLER_H
#define WGFORGE_REQUEST_HANDLER_H

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

  using HandlerFunc = std::function<void(RequestConstRef, ResponseRef)>;

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
  static_assert(std::is_member_function_pointer_v<Method>, "method_ptr must be pointer to method");
  static_assert(std::is_invocable_r_v<void, Method, typename std::add_pointer<Controller>::type,
                                      RequestConstRef, ResponseRef>,
                "method_ptr must be callable on Controller* with (RequestConstRef, ResponseRef) "
                "and return void");
  add_route(method, path, [controller, method_ptr](RequestConstRef req, ResponseRef res) {
	(controller.get()->*method_ptr)(req, res);
  });
}

}  // namespace WgForge

#endif  // WGFORGE_REQUEST_HANDLER_H
