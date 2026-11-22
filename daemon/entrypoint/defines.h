/*
 * defines.h
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

#ifndef WGFORGE_DEFINES_HPP
#define WGFORGE_DEFINES_HPP

#ifndef WGFORGE_SERVER_STRING
  #define WGFORGE_SERVER_STRING "wgforge-server"
#endif

#include <libeconf.h>
#include <wgforge.h>

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <config_manager.hpp>
#include <libwgcpp/wginterface.hpp>

namespace WgForge {

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace json = boost::json;

using Request = http::request<http::string_body>;
using RequestConstRef = const Request&;

using Response = http::response<http::string_body>;
using ResponseRef = Response&;

}  // namespace WgForge

#endif  // WGFORGE_DEFINES_HPP
