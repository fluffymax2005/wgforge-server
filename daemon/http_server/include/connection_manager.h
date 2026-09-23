//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/*
 * connection_manager.h
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

#ifndef __WGFORGE_CONNECTION_MANAGER_H
#define __WGFORGE_CONNECTION_MANAGER_H

#include <set>

#include "connection.h"

namespace WgForge {

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class ConnectionManager {
public:
  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;

  /// Construct a connection manager.
  ConnectionManager();

  /// Add the specified connection to the manager and start it.
  void start(connection_ptr c);

  /// Stop the specified connection.
  void stop(connection_ptr c);

  /// Stop all connections.
  void stop_all();

private:
  /// The managed connections.
  std::set<connection_ptr> connections_;
};

} // namespace WgForge

#endif // __WGFORGE_CONNECTION_MANAGER_H
