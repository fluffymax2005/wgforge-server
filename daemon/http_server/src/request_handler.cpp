//
// RequestHandler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.h"

#include <fstream>
#include <sstream>
#include <string>

namespace WgForge {

void RequestHandler::add_route(http::verb method, const std::string &uri,
                               HandlerFunc handler) {
  routes_.emplace(RouteKey{method, uri}, std::move(handler));
}

void RequestHandler::handle_request(
    const http::request<beast::http::string_body> &req,
    http::response<beast::http::string_body> &res) {

  RouteKey key{req.method(), std::string(req.target())};

  auto handler = routes_.find(key);
  if (handler != routes_.end()) {
    handler->second(req, res);
    return;
  }

  res.result(http::status::not_found);

  res.body() = std::string("Route is not found: ") + req.target().data();
  res.prepare_payload();
}

} // namespace WgForge
