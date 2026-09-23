/*
 * config_manager.h
 * wgforge-core — Core library for wgforge-server (C)
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

#ifndef __WGFORGE_CONFIG_MANAGER_H__
#define __WGFORGE_CONFIG_MANAGER_H__

#include <libeconf.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string_view>

#include "config_read_write_register.h"
#include "wgforge.h"

namespace WgForge {

class ConfigManager {
public:
  using ConfigReturnCode = int;
  using CreateConfigStrategy =
      std::move_only_function<econf_file*(std::string_view path, ConfigReturnCode& rc) noexcept>;

  ConfigManager();
  explicit ConfigManager(std::string_view path, bool readonly = true) noexcept;
  explicit ConfigManager(econf_file* config, bool readonly = true) noexcept;

  template<typename TRegister>
  void setReadWriteRegister(TRegister&& reg);

  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  ConfigManager(ConfigManager&& other) noexcept;
  ConfigManager& operator=(ConfigManager&& other) noexcept;

  ~ConfigManager() = default;

  inline bool isOpen() const noexcept;
  explicit operator bool() const noexcept;

  ConfigReturnCode open(std::string_view path, bool readonly = true);
  inline void close() noexcept;

  void set(econf_file* config, bool readonly = true) noexcept;
  inline econf_file* release() noexcept;

  ConfigReturnCode beginTransaction() noexcept;
  ConfigReturnCode commit() noexcept;
  void rollBack() noexcept;

  inline bool inTransaction() const noexcept;

  ConfigReturnCode create(std::string_view path, CreateConfigStrategy strategy) noexcept;

  template<typename T>
  ConfigWriteRegister::WriteReturnCode write(std::string_view group, std::string_view option,
                                             const T& value);

  template<typename T>
  std::optional<T> tryRead(std::string_view group, std::string_view option,
                           ConfigReadRegister::ReadReturnCode* rc = nullptr) const;

  template<typename T>
  T read(std::string_view group, std::string_view option) const;

private:
  struct ConfigDeleter {
	void operator()(econf_file* p) noexcept;
  };

  std::unique_ptr<econf_file, ConfigDeleter> cfg_;
  std::unique_ptr<econf_file, ConfigDeleter> snapshot_;

  ConfigReadWriteRegister rw_reg_;
  bool readonly_;
  bool in_transaction_ = false;
  mutable std::mutex mtx_;

  void registerDefaultReadersImpl();
  void registerDefaultWritersImpl();
};

template<typename T>
ConfigWriteRegister::WriteReturnCode ConfigManager::write(std::string_view group,
                                                          std::string_view option, const T& value) {
  if (!isOpen())
	return WGFORGE_CONFIG_CLOSED;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_CONFIG_READONLY;

  return rw_reg_.write(cfg_.get(), group, option, value);
}

template<typename TRegister>
void ConfigManager::setReadWriteRegister(TRegister&& reg) {
  std::lock_guard lock(mtx_);
  rw_reg_ = std::forward<TRegister>(reg);
}

template<typename T>
std::optional<T> ConfigManager::tryRead(std::string_view group, std::string_view option,
                                        ConfigReadRegister::ReadReturnCode* rc) const {
  if (!isOpen()) {
	if (rc)
	  *rc = WGFORGE_CONFIG_CLOSED;
	return std::nullopt;
  }

  ConfigReadRegister::ReadReturnCode lrc = 0;
  std::lock_guard lock(mtx_);
  auto result = rw_reg_.tryRead<T>(cfg_.get(), group, option, &lrc);

  if (rc)
	*rc = lrc;
  return result;
}

template<typename T>
T ConfigManager::read(std::string_view group, std::string_view option) const {
  if (!isOpen())
	throw std::runtime_error("Config is not opened");

  return rw_reg_.read<T>(cfg_.get(), group, option);
}

inline bool ConfigManager::isOpen() const noexcept {
  std::lock_guard lock(mtx_);
  return cfg_.get();
}

inline void ConfigManager::close() noexcept {
  std::lock_guard lock(mtx_);
  cfg_.reset();
}

inline econf_file* ConfigManager::release() noexcept {
  std::lock_guard lock(mtx_);
  return cfg_.release();
}

inline bool ConfigManager::inTransaction() const noexcept {
  std::lock_guard lock(mtx_);
  return in_transaction_;
}

} // namespace WgForge

#endif // __WGFORGE_CONFIG_MANAGER_H__
