set(CORE_NAME wgforge-core)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.70 REQUIRED COMPONENTS ALL)
find_package(SQLite3 REQUIRED)
find_package(wgcpp REQUIRED)

add_library(${CORE_NAME} STATIC
    src/sqlite_db_manager.cpp
    src/sqlite_error_handler.cpp
    src/config_error_handler.cpp
    src/config_manager.cpp
    src/peer_config.cpp
    src/subnetscanner.cpp
    src/inetutils.cpp
)

target_include_directories(${CORE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        /usr/include/libnl3/
)

target_link_libraries(${CORE_NAME}
    PUBLIC
       SQLite3::SQLite3
       econf
       wgcpp
)

