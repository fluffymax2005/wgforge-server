/*
 * spinner.hpp
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

#ifndef WGFORGE_CLI_SPINNER_HPP
#define WGFORGE_CLI_SPINNER_HPP

#include <array>
#include <ostream>
#include <stop_token>
#include <string_view>
#include <thread>

namespace WgForge {

class Spinner {
public:
  template<typename Func>
  static void run(std::ostream& os, std::string_view message, Func&& action) {
	// Background thread to display animation
	std::jthread worker([&os, message](std::stop_token stoken) {
	  constexpr std::array<std::string_view, 10> frames{"⠋", "⠙", "⠹", "⠸", "⠼",
	                                                    "⠴", "⠦", "⠧", "⠇", "⠏"};

	  size_t idx = 0;
	  while (!stoken.stop_requested()) {
		// Sequentionnaly display frames. Use \r to get to start of the line
		std::print(os, "\r[{}] {}...", frames[idx % frames.size()], message);
		os.flush();  // forcely display (disable buffering)
		++idx;
		std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_FREQUENCY_MS));
	  }
	});

	try {
	  // Execute target task
	  action();

	  // Stop worker when task is completed
	  worker.request_stop();
	  worker.join();

	  std::println(os, "\r[+] {}... Done!", message);
	} catch (...) {
	  worker.request_stop();
	  worker.join();

	  std::println(os, "\r[x] {}... Failed!", message);
	  throw;
	}
  }

private:
  static constexpr size_t UPDATE_FREQUENCY_MS = 100;
};

}  // namespace WgForge

#endif
