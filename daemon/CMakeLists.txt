cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME wgforge-server)

project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.70 REQUIRED COMPONENTS ALL)
find_package(wgcpp REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/initializer.cpp

    http_server/src/connection.cpp
    http_server/src/connection_manager.cpp
    http_server/src/handlers.cpp
    http_server/src/request_handler.cpp
    http_server/src/wgforgeserver.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    include/
    http_server/include/
    /usr/include/libnl3/
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Boost::boost
        Boost::json
        Boost::url
        SQLite3::SQLite3
        wgcpp
        econf
        nl-3
        nl-route-3
        wgforge-core
        curl
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)



