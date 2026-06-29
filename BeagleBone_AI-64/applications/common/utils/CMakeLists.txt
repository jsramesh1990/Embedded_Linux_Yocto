cmake_minimum_required(VERSION 3.16)
project(utils VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(LIBSERIAL REQUIRED libserialport)

# Library sources
set(UTILS_SOURCES
    src/StringUtils.cpp
    src/FileUtils.cpp
    src/TimeUtils.cpp
    src/MathUtils.cpp
    src/NetworkUtils.cpp
    src/SystemUtils.cpp
    src/ThreadPool.cpp
    src/CircularBuffer.cpp
    src/JsonUtils.cpp
    src/CryptoUtils.cpp
    src/SerialPort.cpp
    src/SignalHandler.cpp
)

set(UTILS_HEADERS
    include/utils/StringUtils.h
    include/utils/FileUtils.h
    include/utils/TimeUtils.h
    include/utils/MathUtils.h
    include/utils/NetworkUtils.h
    include/utils/SystemUtils.h
    include/utils/ThreadPool.h
    include/utils/CircularBuffer.h
    include/utils/JsonUtils.h
    include/utils/CryptoUtils.h
    include/utils/SerialPort.h
    include/utils/SignalHandler.h
)

# Create library
add_library(utils STATIC ${UTILS_SOURCES})

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${LIBSERIAL_INCLUDE_DIRS}
)

target_compile_definitions(utils PRIVATE
    -DUTILS_USE_OPENSSL
    -DUTILS_USE_SERIAL
)

target_link_libraries(utils
    ${JSONCPP_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSERIAL_LIBRARIES}
    pthread
    rt
    dl
)

# Install
install(TARGETS utils DESTINATION lib)
install(FILES ${UTILS_HEADERS} DESTINATION include/utils)

# Tests
option(BUILD_UTILS_TESTS "Build utils tests" ON)
if(BUILD_UTILS_TESTS)
    find_package(GTest REQUIRED)
    add_executable(utils_test
        tests/UtilsTest.cpp
        tests/StringUtilsTest.cpp
        tests/FileUtilsTest.cpp
        tests/TimeUtilsTest.cpp
        tests/MathUtilsTest.cpp
        tests/NetworkUtilsTest.cpp
        tests/SystemUtilsTest.cpp
        tests/ThreadPoolTest.cpp
        tests/CircularBufferTest.cpp
        tests/JsonUtilsTest.cpp
        tests/CryptoUtilsTest.cpp
    )
    target_link_libraries(utils_test
        utils
        GTest::GTest
        GTest::Main
    )
    target_include_directories(utils_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME utils_test COMMAND utils_test)
endif()
