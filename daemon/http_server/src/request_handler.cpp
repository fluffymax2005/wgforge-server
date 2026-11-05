//
// RequestHandler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/*
 *  request_handler.cpp
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

#include "request_handler.h"

#include <boost/url/parse.hpp>
#include <fstream>
#include <sstream>
#include <string>

namespace WgForge {

void RequestHandler::add_route(http::verb method, const std::string& uri, HandlerFunc handler) {
  routes_.emplace(RouteKey{method, uri}, std::move(handler));
}

void RequestHandler::handle_request(const http::request<beast::http::string_body>& req,
                                    http::response<beast::http::string_body>& res) {
  // Request path from possible query
  auto r = boost::urls::parse_origin_form(req.target());
  std::string path;
  if (r) {
	path = r->path();
  } else {
	path = std::string(req.target());
	auto q = path.find('?');
	if (q != std::string::npos)
	  path.resize(q);
  }

  RouteKey key{req.method(), std::move(path)};

  auto handler = routes_.find(key);
  if (handler != routes_.end()) {
	handler->second(req, res);
	return;
  }

  res.result(http::status::not_found);

  res.body() = std::string("Route is not found: ") + req.target().data();
  res.prepare_payload();
}

}  // namespace WgForge
