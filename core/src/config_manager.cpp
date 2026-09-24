/*
 * config_manager.cpp
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

#include "config_manager.h"

#include <filesystem>

#include "config_read_functions.h"
#include "config_write_functions.h"

namespace WgForge {

ConfigManager::ConfigManager() {
  registerDefaultReadersImpl();
  registerDefaultWritersImpl();
}

void ConfigManager::registerDefaultReadersImpl() {
  rw_reg_.registerReader<bool>(LibeconfReaders::readBool);
  rw_reg_.registerReader<int>(LibeconfReaders::readInt);
  rw_reg_.registerReader<int64_t>(LibeconfReaders::readInt64);
  rw_reg_.registerReader<uint32_t>(LibeconfReaders::readUInt);
  rw_reg_.registerReader<uint64_t>(LibeconfReaders::readUInt64);
  rw_reg_.registerReader<std::string>(LibeconfReaders::readString);
  rw_reg_.registerReader<float>(LibeconfReaders::readFloat);
  rw_reg_.registerReader<double>(LibeconfReaders::readDouble);
}

void ConfigManager::registerDefaultWritersImpl() {
  rw_reg_.registerWriter<bool>(LibeconfWriters::writeBool);
  rw_reg_.registerWriter<int>(LibeconfWriters::writeInt);
  rw_reg_.registerWriter<int64_t>(LibeconfWriters::writeInt64);
  rw_reg_.registerWriter<uint32_t>(LibeconfWriters::writeUInt);
  rw_reg_.registerWriter<uint64_t>(LibeconfWriters::writeUInt64);
  rw_reg_.registerWriter<std::string>(LibeconfWriters::writeString);
  rw_reg_.registerWriter<std::string_view>(LibeconfWriters::writeStringView);
  rw_reg_.registerWriter<float>(LibeconfWriters::writeFloat);
  rw_reg_.registerWriter<double>(LibeconfWriters::writeDouble);
}

ConfigManager::ConfigManager(std::string_view path, bool readonly) noexcept {
  open(path, readonly);
}

ConfigManager::ConfigManager(econf_file* config, bool readonly) noexcept {
  set(config, readonly);
}

ConfigManager::ConfigManager(ConfigManager&& other) noexcept {
  if (this != &other) {
	std::lock_guard lock(other.mtx_);

	readonly_ = other.readonly_;
	rw_reg_ = std::move(rw_reg_);
	cfg_ = std::move(other.cfg_);
  }
}

ConfigManager& ConfigManager::operator=(ConfigManager&& other) noexcept {
  if (this != &other) {
	std::scoped_lock lock(mtx_, other.mtx_);

	readonly_ = other.readonly_;
	rw_reg_ = std::move(rw_reg_);
	cfg_ = std::move(other.cfg_);
  }

  return *this;
}

ConfigManager::ConfigReturnCode ConfigManager::beginTransaction() noexcept {
  if (!isOpen())
	return WGFORGE_CONFIG_CLOSED;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_CONFIG_READONLY;
  if (in_transaction_)
	return WGFORGE_CONFIG_TRANSACTION_ACTIVE;

  // Get config full path
  char* raw_path = econf_getPath(cfg_.get());
  if (raw_path == nullptr)
	return WGFORGE_CONFIG_NOMEM;
  std::unique_ptr<char, decltype(&free)> path_guard(raw_path, &free);

  // Read file for copy
  econf_file* copy;
  econf_err ec = econf_readFile(&copy, raw_path, "=", "#");
  if (ec != ECONF_SUCCESS)
	return WGFORGE_CONFIG_NOMEM;

  snapshot_.reset(copy);
  in_transaction_ = true;
  return WGFORGE_CONFIG_OK;
}

ConfigManager::ConfigReturnCode ConfigManager::commit() noexcept {
  namespace fs = std::filesystem;

  if (!isOpen())
	return WGFORGE_CONFIG_CLOSED;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_CONFIG_READONLY;
  if (!in_transaction_)
	return WGFORGE_CONFIG_TRANSACTION_INACTIVE;

  // Get fullpath
  char* raw_path = econf_getPath(cfg_.get());
  if (raw_path == nullptr)
	return WGFORGE_CONFIG_NOMEM;
  std::unique_ptr<char, decltype(&free)> path_guard(raw_path, &free);

  fs::path path(raw_path);
  fs::path tmp_path = path.parent_path() / (path.filename().string() + ".tmp");

  // Write into tmp file
  econf_err ec =
      econf_writeFile(cfg_.get(), tmp_path.parent_path().c_str(), tmp_path.filename().c_str());
  if (ec != ECONF_SUCCESS)
	fs::remove(tmp_path);
  return WGFORGE_CONFIG_RUNTIME_ERROR;

  std::error_code fs_ec;
  fs::rename(tmp_path, path, fs_ec);

  if (fs_ec) {
	fs::remove(tmp_path);
	return WGFORGE_CONFIG_RUNTIME_ERROR;
  }

  snapshot_.reset();
  in_transaction_ = false;
  return WGFORGE_CONFIG_OK;
}

ConfigManager::ConfigReturnCode ConfigManager::rollBack() noexcept {
  std::lock_guard lock(mtx_);

  if (!in_transaction_)
	return WGFORGE_CONFIG_TRANSACTION_INACTIVE;

  cfg_ = std::move(snapshot_);
  in_transaction_ = false;
  return WGFORGE_CONFIG_OK;
}

ConfigManager::operator bool() const noexcept {
  return isOpen();
}

ConfigManager::ConfigReturnCode ConfigManager::open(std::string_view path, bool readonly) {
  std::lock_guard lock(mtx_);
  econf_file* config = cfg_.get();
  econf_err ec = econf_readFile(&config, path.data(), "=", "#");
  if (ec == ECONF_SUCCESS) {
	readonly_ = readonly;
	cfg_.reset(config);
  }
  return ec;
}

void ConfigManager::set(econf_file* context, bool readonly) noexcept {
  std::lock_guard lock(mtx_);
  cfg_.reset(context);
  readonly_ = readonly;
}

ConfigManager::ConfigReturnCode ConfigManager::create(std::string_view path,
                                                      CreateConfigStrategy strategy) noexcept {
  if (!strategy)
	return WGFORGE_CONFIG_STRATEGY_NO_TARGET;

  ConfigReturnCode rc;
  econf_file* config = strategy(path, rc);
  if (rc != ECONF_SUCCESS || config == nullptr)
	return rc;

  std::lock_guard lock(mtx_);
  cfg_.reset(config);
  return rc;
}

void ConfigManager::ConfigDeleter::operator()(econf_file* p) noexcept {
  if (p)
	econf_freeFile(p);
}

} // namespace WgForge
