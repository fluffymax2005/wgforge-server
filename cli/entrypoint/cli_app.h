/*
 * cli_app.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_CLI_APP_H
#define WGFORGE_CLI_APP_H

#include <ostream>

#include "CLI/CLI.hpp"
#include "cli_controller.h"

namespace WgForge {

class CliApp {
public:
  explicit CliApp(std::shared_ptr<CliController> cli_controller, std::ostream& os = std::cout);

  int run(int argc, char* argv[]);

private:
  std::shared_ptr<CliController> cli_controller_;
  std::ostream& os_;
  CLI::App app_;

  void setupOptions();
};

}  // namespace WgForge

#endif  // WGFORGE_CLI_APP_H
