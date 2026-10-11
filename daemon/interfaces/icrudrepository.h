/*
 * icrudrepository.h
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

#ifndef __WGFORGE_ICRUDREPOSITORY_H__
#define __WGFORGE_ICRUDREPOSITORY_H__

#include <future>
#include <optional>
#include <vector>

namespace WgForge {

template<typename T, typename Key>
class ICrudRepositoryAsync {
public:
  virtual ~ICrudRepositoryAsync() = default;

  virtual std::future<std::optional<T>> getAsync(const Key& key) const = 0;
  virtual std::future<std::optional<std::vector<T>>> getAllAsync() const = 0;
  virtual std::future<void> createAsync(const T& inst) = 0;
  virtual std::future<void> updateAsync(const T& inst) = 0;
  virtual std::future<void> softDeleteAsync(const Key& key) = 0;
  virtual std::future<void> hardDeleteAsync(const Key& key) = 0;
};

}  // namespace WgForge


#endif  // __WGFORGE_ICRUDREPOSITORY_H__
