/*
 * time_utils.hpp
 * wgforge-core — Core library for wgforge-server (C) and wgforge-cli (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-core is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-core. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_TIME_UTILS_HPP
#define WGFORGE_TIME_UTILS_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace WgForge {

inline void validateIso8601Strptime(const std::string& date_str) {
  std::tm tm = {};

  // strptime быстро сканирует строку по указателю
  char* res = strptime(date_str.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
  if (!res || *res != '\0') {
	// Пробуем формат с пробелом вместо T
	res = strptime(date_str.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
  }

  if (!res || *res != '\0') {
	throw std::invalid_argument("Invalid ISO 8601 date format");
  }

  // timegm преобразует tm в UTC timestamp
  std::time_t expire_time = timegm(&tm);
  if (expire_time == -1 || expire_time <= std::time(nullptr)) {
	throw std::invalid_argument("Expiration date must be in the future");
  }
}

inline std::string getCurrentUtcTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

}  // namespace WgForge

#endif
