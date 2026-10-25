/*
 * ilogger.h
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

#ifndef WGFORGE_ILOGGER_H
#define WGFORGE_ILOGGER_H

#include <string>

namespace WgForge {

class ILogger {
public:
  virtual ~ILogger() = default;
  virtual void info(const std::string& msg) = 0;
  virtual void error(const std::string& msg) = 0;
  virtual void warn(const std::string& msg) = 0;
};

}  // namespace WgForge

#endif  // WGFORGE_ILOGGER_H
