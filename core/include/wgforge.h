/*
 * wgforge.h
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

#ifndef __WGFORGE_H__
#define __WGFORGE_H__

#include <arpa/inet.h>
#include <libeconf.h>
#include <sqlite3.h>

#include <wginterface.hpp>
#include <wgprivatekey.hpp>

/**
 * @brief Thread safety type for libwgcpp. Default one is MultiThreaded.
 */
#define __THREAD_SAFETY__ MultiThreaded

namespace WgForge {

// SQLiteDbManager

#define WGFORGE_DATABASE_OK                 SQLITE_OK  ///< Database operation succeded
#define WGFORGE_INVALID_ARGUMENT            -1         ///< Invalid argument provided
#define WGFORGE_RUNTIME_ERROR               -2         ///< Runtime error occured
#define WGFORGE_RECORD_EXIST                -3         ///< Record already exists
#define WGFORGE_CONNECTION_CLOSED           -4         ///< Database connection closed
#define WGFORGE_PEER_EXIST                  -5         ///< Peer already exists
#define WGFORGE_PEER_NOT_EXIST              -6         ///< Peer doesn't exist
#define WGFORGE_ALLOWEDIP_EXIST             -7         ///< Allowedip already exists
#define WGFORGE_ALLOWEDIP_NOT_EXIST         -8         ///< Allowedip doesn't exist
#define WGFORGE_DATABASE_READONLY           -9         ///< Database opened in readonly mode
#define WGFORGE_DATABASE_STRATEGY_NO_TARGET -10        ///< Empty strategy (functor) provided

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

// Core structures

/**
 * @brief Wireguard iface configuration
 */
struct InterfaceConfig {
  char addr[INET_ADDRSTRLEN];  ///< IPv4 address
  WgPrivateKey private_key;    ///< Private key
  char name[IFNAMSIZ];         ///< Name
  in_port_t listenPort;        ///< Listen port
  Protocol proto_type;         ///< Protocol type listening on. Reserved for future
  bool checkSubnet;            ///< Whether daemon check subnet occupation on startup
};

/**
 * @brief HTTP server configuration
 */
struct ServerConfig {
  char addr[INET_ADDRSTRLEN];  ///< Listening address
  in_port_t listenPort;        ///< Listen port
};

}  // namespace WgForge

#endif
