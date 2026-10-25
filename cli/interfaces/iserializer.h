/*
 * iserializer.h
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
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

#ifndef WGFORGE_ISERIALIZER_H
#define WGFORGE_ISERIALIZER_H

#include <string>

namespace WgForge {

template<typename Entity>
class ISerializer {
public:
  virtual ~ISerializer() = default;

  virtual std::string serialize(const Entity& entity) const = 0;
  virtual Entity deserialize(const std::string& entity) const = 0;
};

}  // namespace WgForge


#endif  // WGFORGE_ISERIALIZER_H
