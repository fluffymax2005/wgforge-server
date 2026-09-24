/*
 * config_read_write_register.h
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

#ifndef __WGFORGE_READ_WRITE_REGISTER_H__
#define __WGFORGE_READ_WRITE_REGISTER_H__

#include "config_read_register.h"
#include "config_write_register.h"

namespace WgForge {

/**
 * @brief Inheritor to perfomr IO config operations.
 */
class ConfigReadWriteRegister : public ConfigReadRegister, public ConfigWriteRegister {
public:
  ConfigReadWriteRegister() = default;

  ConfigReadWriteRegister(const ConfigReadWriteRegister&) = default;
  ConfigReadWriteRegister& operator=(const ConfigReadWriteRegister&) = default;

  ConfigReadWriteRegister(ConfigReadWriteRegister&&) = default;
  ConfigReadWriteRegister& operator=(ConfigReadWriteRegister&&) = default;
};

} // namespace WgForge

#endif // __WGFORGE_READ_WRITE_REGISTER_H__
