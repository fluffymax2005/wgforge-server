/*
 * iconfiguration.h
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

#ifndef __WGFORGE_ICONFIGURATION_H__
#define __WGFORGE_ICONFIGURATION_H__

#include <string>
#include <string_view>

namespace WgForge {

class IConfigurationSection;

class IConfiguration {
public:
  virtual ~IConfiguration() = default;

  virtual std::string& operator[](std::string_view key) = 0;
  virtual const std::string& get(std::string_view key) const = 0;
  virtual void set(std::string_view key, std::string_view value) = 0;
  virtual IConfigurationSection& getSection(std::string_view name) = 0;
};

}  // namespace WgForge

#endif  // __WGFORGE_ICONFIGURATION_H__
