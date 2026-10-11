/*
 * configuration_section.cpp 
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

#include "configuration_section.h"

#include <format>

namespace WgForge {

ConfigurationSection::ConfigurationSection(std::string key, std::string path)
    : key_(std::move(key)), path_(std::move(path)) {
}

std::string_view ConfigurationSection::key() const {
  return key_;
}

std::string_view ConfigurationSection::path() const {
  return path_;
}

std::string_view ConfigurationSection::value() const {
  return get("");
}

std::string& ConfigurationSection::operator[](std::string_view key) {
  return values_[std::string(key)];
}

const std::string& ConfigurationSection::get(std::string_view key) const {
  auto it = values_.find(std::string(key));
  if (it == values_.end())
	throw std::runtime_error("Value not found");
  return it->second;
}

void ConfigurationSection::set(std::string_view key, std::string_view value) {
  values_[std::string(key)] = std::string(value);
}

IConfigurationSection& ConfigurationSection::getSection(std::string_view name) {
  std::string child_key(name);
  if (children_.find(child_key) == children_.end()) {
	std::string child_path = path_.empty() ? child_key : std::format("{}:{}", path_, child_key);
	children_.emplace(child_key, std::make_unique<ConfigurationSection>(child_key, child_path));
  }
  return *children_[child_key];
}


}  // namespace WgForge
