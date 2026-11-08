/*
 * wgforge.h
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

#ifndef WGFORGE_H
#define WGFORGE_H

#include <arpa/inet.h>
#include <libeconf.h>
#include <sqlite3.h>

#include <wginterface.hpp>
#include <wgprivatekey.hpp>

// Some cli option should be ignores for Windows Family
#if defined(_WIN32) || defined(_WIN64)
  #define WGFORGE_CLI_WINDOWS
#endif

// As well as for MacOS
#ifdef __MACOS__
  #define WGFORGE_CLI_MACOS
#endif

#ifdef __linux__
  #define WGFORGE_CLI_LINUX
#endif

/**
 * @brief Thread safety type for libwgcpp. Default one is SingleThreaded.
 */
#define __THREAD_SAFETY__ SingleThreaded

#ifdef WGFORGE_CLI_WINDOWS
  #define ENDLINE "\r\n"
#else
  #define ENDLINE "\n"
#endif

namespace WgForge {

// ConfigManager

#define WGFORGE_CONFIG_OK                   ECONF_SUCCESS  ///< Config operation succeded
#define WGFORGE_CONFIG_READONLY             -100           ///< Config opened in readonly mode
#define WGFORGE_CONFIG_CLOSED               -101           ///< Config is closed
#define WGFORGE_CONFIG_STRATEGY_NO_TARGET   -102           ///< Empty strategy (functor) provided
#define WGFORGE_CONFIG_NOMEM                -103  ///< Not enough memory to handle operation
#define WGFORGE_CONFIG_TRANSACTION_ACTIVE   -104  ///< Config is in transaction already
#define WGFORGE_CONFIG_TRANSACTION_INACTIVE -105  ///< Config isn't in transaction yet
#define WGFORGE_CONFIG_RUNTIME_ERROR        -106  ///< Runtime error occured

// ConfigWriteRegister

#define WGFORGE_CONFIG_WRITER_NOT_FOUND     -200  ///< Config writer not found

// ConfigReadRegister

#define WGFORGE_CONFIG_READER_NOT_FOUND     -300  ///< Config reader not found
#define WGFORGE_CONFIG_INVALID_TYPE         -301  ///< Invalid type provided for config reader

// Typedefs

using WgInterface = WgInterface<__THREAD_SAFETY__>;
using WgPeer = WgPeer<__THREAD_SAFETY__>;
using WgKey = WgKey<__THREAD_SAFETY__>;
using WgPrivateKey = WgPrivateKey<__THREAD_SAFETY__>;
using WgPublicKey = WgPublicKey<__THREAD_SAFETY__>;
using WgPresharedKey = WgPresharedKey<__THREAD_SAFETY__>;
using WgAllowedIP = WgAllowedIP<__THREAD_SAFETY__>;
using WgEndpoint = WgEndpoint<__THREAD_SAFETY__>;

}  // namespace WgForge

#endif  // WGFORGE_H
