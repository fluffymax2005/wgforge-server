//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __WGFORGE_REQUEST_HANDLER_H__
#define __WGFORGE_REQUEST_HANDLER_H__

#include <boost/beast/http.hpp>
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

  private:
  std::unordered_map<RouteKey, HandlerFunc, RouteKeyHash> routes_;
};

} // namespace WgForge

#endif // __WGFORGE_REQUEST_HANDLER_H__
