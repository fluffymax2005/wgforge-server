cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME_CORE wgforge)

project(${PROJECT_NAME_CORE} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(common)
#add_subdirectory(cli)
add_subdirectory(daemon)


