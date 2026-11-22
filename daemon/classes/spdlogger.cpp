/*
 * spdlogger.cpp
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

#include "spdlogger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "defines.h"

namespace WgForge {

SpdLogger::SpdLogger() {
  logger_ = spdlog::stdout_color_mt("wgforge-server");
  logger_->set_pattern(
      std::format("[%Y-%m-%d %H:%M:%S.%e] {}[%t] [%^%l%$]: %v", WGFORGE_SERVER_STRING));
}

void SpdLogger::info(const std::string& msg) {
  logger_->info(msg);
}

void SpdLogger::error(const std::string& msg) {
  logger_->error(msg);
}

void SpdLogger::warn(const std::string& msg) {
  logger_->warn(msg);
}

void SpdLogger::critical(const std::string& msg) {
  logger_->critical(msg);
}

}  // namespace WgForge
