set(COMMON_NAME wgforge-common)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.70 REQUIRED COMPONENTS ALL)
find_package(SQLite3 REQUIRED)
find_package(wgcpp REQUIRED)

add_library(${COMMON_NAME} STATIC
    src/sqlite_db_manager.cpp
    src/sqlite_error_handler.cpp
    src/config_error_handler.cpp
    src/config_manager.cpp
)



target_include_directories(${COMMON_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${COMMON_NAME}
    PUBLIC
       SQLite3::SQLite3
       econf
       wgcpp
)

