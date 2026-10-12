/*
 * iexceptionhandler.h
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

#ifndef __WGFORGE_IEXCEPTIONHANDLE_H__
#define __WGFORGE_IEXCEPTIONHANDLE_H__

#include <exception>

#include "defines.h"

namespace WgForge {

class IExceptionHandle {
public:
  virtual ~IExceptionHandle() = default;

  virtual void handleException(std::exception_ptr ep, Request req, Response& res) = 0;
};

}  // namespace WgForge

#endif  // __WGFORGE_IEXCEPTIONHANDLE_H__
