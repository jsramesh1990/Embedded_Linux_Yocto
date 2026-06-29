cmake_minimum_required(VERSION 3.16)
project(config VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create library
add_library(config STATIC
    src/ConfigManager.cpp
    src/ConfigSchema.cpp
)

target_include_directories(config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

target_include_directories(config PRIVATE
    ${YAML_CPP_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(config
    ${YAML_CPP_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)

# Install
install(TARGETS config DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
