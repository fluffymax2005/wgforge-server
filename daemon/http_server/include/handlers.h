/*
 * handlers.h
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

#ifndef __WGFORGE_HANDLERS_H__
#define __WGFORGE_HANDLERS_H__

#include <iostream>

#include "defines.h"

namespace WgForge {

// GET
void interface__get(Request req, Response res, EndpointArgs& args);
void peers(Request req, Response res, EndpointArgs& args);
void peer__get(Request req, Response res, EndpointArgs& args);
void peer_config(Request req, Response res, EndpointArgs& args);

// POST
void peer__post(Request req, Response res, EndpointArgs& args);

// PATCH
void interface__patch(Request req, Response res, EndpointArgs& args);
void peer__patch(Request req, Response res, EndpointArgs& args);
void peer_rotate(Request req, Response res, EndpointArgs& args);

// DELETE
void peer__delete(Request req, Response res, EndpointArgs& args);

// Utils
template<typename... Args>
void log_info_to(std::ostream& os, Args&&... args) {
  os << BLUE_COLOR << "[INFO]" << RESET_COLOR << " WgForge: ";
  (os << ... << args);
  os << std::endl;
}

template<typename... Args>
void log_info(Args&&... args) {
  log_info_to(std::clog, args...);
}

template<typename... Args>
void log_error_to(std::ostream& os, Args&&... args) {
  os << RED_COLOR << "[ERROR]" << RESET_COLOR << " WgForge: ";
  (os << ... << args);
  os << std::endl;
}

template<typename... Args>
void log_error(Args&&... args) {
  log_error_to(std::cerr, args...);
}

} // namespace WgForge

#endif // __WGFORGE_HANDLERS_H__
