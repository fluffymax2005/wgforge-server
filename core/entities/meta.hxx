/*
 * meta.hxx
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

#ifndef WGFORGE_META_HXX
#define WGFORGE_META_HXX

#include <string>

namespace WgForge {

/**
 * Meta information to support proper peers' data in case of changing core interface config
 */

#pragma db object no_id table("Meta")

struct Meta {
#pragma db column("Interface_address") type("TEXT")
  std::string iface_address;
};

}  // namespace WgForge

#endif  // WGFORGE_META_HXX
