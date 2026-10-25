cmake_minimum_required(VERSION 4.3.1)

set(PROJECT_NAME_CORE wgforge)

project(${PROJECT_NAME_CORE} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)


add_subdirectory(core)
add_subdirectory(contracts)
add_subdirectory(cli)
add_subdirectory(daemon)



