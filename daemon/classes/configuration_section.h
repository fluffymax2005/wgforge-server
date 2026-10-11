/*
 * configuration_section.h 
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

#ifndef __WGFORGE_CONFIGURATION_SECTION_H__
#define __WGFORGE_CONFIGURATION_SECTION_H__

#include <memory>
#include <string>
#include <unordered_map>

#include "iconfiguration_section.h"

namespace WgForge {

class ConfigurationSection : public IConfigurationSection {
public:
  ConfigurationSection(std::string key, std::string path);

  virtual std::string_view key() const override;
  virtual std::string_view path() const override;
  virtual std::string_view value() const override;

  virtual std::string& operator[](std::string_view key) override;
  virtual const std::string& get(std::string_view key) const override;
  virtual void set(std::string_view key, std::string_view value) override;
  virtual IConfigurationSection& getSection(std::string_view name) override;

protected:
  std::string key_;
  std::string path_;
  std::unordered_map<std::string, std::string> values_;
  std::unordered_map<std::string, std::unique_ptr<ConfigurationSection>> children_;
};

}  // namespace WgForge


#endif  // __WGFORGE_CONFIGURATION_SECTION_H__
