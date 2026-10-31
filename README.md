<p align="center">
  <img align="center" src="https://raw.github.com/fluffymax2005/wgforge-server/40800c94bd5af2516467f6c01f91a3ce1827a9fa/images/logo.jpg">
</p>

# Contents

1. [Introduction](#introduction)
2. [Features](#features)
3. [Dependencies](#dependencies)
   - [WgForge Server](#wgforge-server)
   - [WgForge CLI](#wgforge-cli)
4. [Build](#build)
   - [Linux](#linux)
   - [MacOS](#macos)
   - [Windows](#windows)
5. [Installation](#installation)
6. [Documentation](#documentation):
   - [C++](#c++)
   - [HTTP API](#http-api)
7. [Contributing](#contributing)


# Introduction

WgForge - project aimed to provide tools to easily deploy Wireguard server on any Linux based distro as well as administrate it via CLI or [GUI](https://github.com/fluffymax2005/wgforge-gui) on any platform - Windows, Linux, MacOS. It takes most annoying routine to manage Wireguard server: CRUD peers, archive them

# Features

WgForge Server:
- **Systemd** integration as template service
- Suitable for **embeddable** systems. The SQLite3 database is used to store peers' configuration by ```/var/lib/wgforge/```
- **Logging**. Includes **sensitive logs** such as **rotating** peer's public key redirecting to systemd logs
- **Customization**. Administrator can **configure HTTP server** and **Wireguard interface** via .conf files in ```/etc/wgforge/```
- Wireguard based **config generator**. One can request generate config for concrete peer and get .txt based configuration which can you directly. In addition, requester may override default public server IP address

WgForge CLI:
- Simple **stable API** to manage daemon
- Considers **features** of **different OS**. For example, Windows clients cannot locally read HTTP server config but rather connect to server by SSH and read it then
- **Various data output variations**. Peers' data may be printed as a table or as a list. It may be useful for scripts

# Dependencies

## WgForge Core

**WgForge Core** is a core layer which provides obligating data for **WgForge Server** (aka daemon) and **WgForge CLI**. It is not intended to be built as standalone binary, but rather to be linked as a static library

- [libodb core](https://github.com/Riebart/libodb)
- [libodb runtime](https://codesynthesis.com/products/odb/)
- [libodb sqlite](https://codesynthesis.com/products/odb/) (SQLite3 DB provider)
- [libeconf](https://github.com/openSUSE/libeconf)
- [libwgcpp](https://github.com/fluffymax2005/libwgcpp) (self written C++ wrapper over embeddable-wg-library)

## WgForge Server

- [Boost.Core](https://www.boost.org/library/latest/core/)
- [Boost.Json](https://www.boost.org/library/latest/json/)
- [Boost.Url](https://www.boost.org/library/latest/url/)
- [Boost.DI (Boost.Ext)](https://boost-ext.github.io/di/)
- [libnl3](https://github.com/thom311/libnl/)
- [libcurl](https://curl.se/libcurl/)
- [spdlog](https://spdlog.readthedocs.io/en/latest/#documentation)
- [fmt](https://github.com/fmtlib/fmt)


## WgForge CLI

- [Boost.Json](https://www.boost.org/library/latest/json/)
- [Boost.Url](https://www.boost.org/library/latest/url/)
- [Boost.DI (Boost.Ext)](https://boost-ext.github.io/di/)
- [CLI11](https://github.com/CLIUtils/CLI11)


# Build

1. Requirements:
   - GCC: version 13+
   - Clang: version 16+
   - MSVC: Visual Studio 2022 (version 17.5+)
1. CMake: version 3.28+
3. Make (recommended for Linux) or MSBuild

As project uses submodules (Boost.DI locally) clone repo with --recursive:
```
git clone --recursive -b master https://github.com/fluffymax2005/wgforge-server.git
cd wgforge-server
```

## Linux

Generate build files for both **WgForge CLI** and **WgForge Server** for **Make**:
```
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX=/usr
```

Build **WgForge CLI**:
```
cmake --build build --target wgforge-cli -j$(nproc)
```

Build **WgForge Server**:
```
cmake --build build --target wgforge-server -j$(nproc)
```

## MacOS

Generate build files for both **WgForge CLI** for **Make**:
```
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX=/usr
```

Build **WgForge CLI**:
```
cmake --build build --target wgforge-cli -j$(sysctl -n hw.ncpu)
```

## Windows

Generate build files for both **WgForge CLI**:
```
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=MinSizeRel -A x64 -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

Build **WgForge CLI**:
```
cmake --build build --target wgforge-cli
```

# Installation

## Option 1: Install from pre-built binaries (recommended)

Download the latest '.deb' or '.rpm' packages from the [Releases](https://github.com/fluffymax2005/wgforge-server/releases) page

### Ubuntu / Debian:

```bash
sudo apt install ./wgforge-server_1.0.0_amd64.deb
sudo apt install ./wgforge-cli_1.0.0_amd64.deb
```

### Fedora / RHEL:

```bash
sudo dnf install ./wgforge-server_1.0.0-x86_64.rpm
sudo dnf install ./wgforge-cli_1.0.0_amd64.rpm
```

## Option 2: Install from source

Make sure you have built the project before installing.
Install all components (CLI + Server):
```bash
sudo cmake --install build
```

Install CLI only:
```bash
sudo cmake --install build --component wgforge-cli
```

Install Server only (Linux):
```bash
sudo cmake --install build --component wgforge-server
```



# Documentation

## C++

Most C++ classes from different layers (e.g. core) are supplied with Doxygen documentation. Also most IDEs show up hints provided by classes. In case of see web based Doxygen base documentation execute from root projects directory:
```
cd docs
doxygen
```


## HTTP API

HTTP Server API is written in OpenAPI 3.1.0 by path ```docs/wgforge-server.yaml```

# Contributing

Found any issues or want to help to distribute package into other Linux distros? Create [issue](https://github.com/fluffymax2005/libwgcpp/issues/new) and tell about with template:
- Brief: description of vulnerability or suggestions for improvement
- Touches file(s): where you found it
- Explanation: your explanation
- (Optional) Your suggestions to improve project: what should be done