/*
 * endpoint_runtime_error.h
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

#ifndef WGFORGE_ENDPOINT_RUNTIME_ERROR_H
#define WGFORGE_ENDPOINT_RUNTIME_ERROR_H

#include <stdexcept>

namespace WgForge {

class EndpointRuntimeError : public std::runtime_error {
public:
  EndpointRuntimeError(const std::string& msg, const std::string& endpoint,
                       const std::string& method)
      : std::runtime_error(msg), endpoint_uri(endpoint), method(method) {}

  EndpointRuntimeError(const char* msg, const char* endpoint, const char* method)
      : std::runtime_error(msg), endpoint_uri(endpoint), method(method) {}

  const std::string& getUri() const noexcept { return endpoint_uri; }

  const std::string& getMethod() const noexcept { return method; }

private:
  std::string endpoint_uri;
  std::string method;
};

}  // namespace WgForge

#endif  // WGFORGE_ENDPOINT_RUNTIME_ERROR_H
