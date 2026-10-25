/*
 * http_request.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_HTTP_REQUEST_H
#define WGFORGE_HTTP_REQUEST_H

#include <string>
#include <utility>
#include <vector>

namespace WgForge {

struct HttpRequest {
  std::string method;
  std::string target;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

}  // namespace WgForge


#endif  // WGFORGE_HTTP_REQUEST_H
