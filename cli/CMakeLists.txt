set(CLI_NAME wgforge-cli)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.70 REQUIRED COMPONENTS ALL)

add_executable(${CLI_NAME} 
    src/main.cpp
)

target_link_libraries(${CLI_NAME}
    PRIVATE
        wgforge_common
        Boost::program_options
)
