cmake_minimum_required(VERSION 3.16)
project(logger VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

# Library sources
set(LOGGER_SOURCES
    src/Logger.cpp
    src/ConsoleSink.cpp
    src/FileSink.cpp
    src/SyslogSink.cpp
    src/NetworkSink.cpp
    src/RotatingFileSink.cpp
)

set(LOGGER_HEADERS
    include/logger/Logger.h
    include/logger/LogLevel.h
    include/logger/LogSink.h
    include/logger/ConsoleSink.h
    include/logger/FileSink.h
    include/logger/SyslogSink.h
    include/logger/NetworkSink.h
    include/logger/RotatingFileSink.h
)

# Create library
add_library(logger STATIC ${LOGGER_SOURCES})

target_include_directories(logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIRS}
)

target_compile_definitions(logger PRIVATE
    -DLOGGER_USE_SYSLOG
    -DLOGGER_USE_NETWORK
)

target_link_libraries(logger
    ${JSONCPP_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
    pthread
    rt
)

# Conditionally link syslog
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(logger PRIVATE -DLOGGER_HAVE_SYSLOG)
endif()

# Install
install(TARGETS logger DESTINATION lib)
install(FILES ${LOGGER_HEADERS} DESTINATION include/logger)

# Tests
option(BUILD_LOGGER_TESTS "Build logger tests" ON)
if(BUILD_LOGGER_TESTS)
    find_package(GTest REQUIRED)
    add_executable(logger_test
        tests/LoggerTest.cpp
        tests/LogSinkTest.cpp
        tests/PerformanceTest.cpp
    )
    target_link_libraries(logger_test
        logger
        GTest::GTest
        GTest::Main
    )
    target_include_directories(logger_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME logger_test COMMAND logger_test)
endif()
