cmake_minimum_required(VERSION 3.16)
project(ipc VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Library sources
set(IPC_SOURCES
    src/IpcManager.cpp
    src/IpcMessage.cpp
    src/IpcClient.cpp
    src/IpcServer.cpp
)

set(IPC_HEADERS
    include/ipc/IpcManager.h
    include/ipc/IpcMessage.h
    include/ipc/IpcClient.h
    include/ipc/IpcServer.h
)

# Create library
add_library(ipc STATIC ${IPC_SOURCES})

target_include_directories(ipc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
)

target_compile_definitions(ipc PRIVATE
    -DIPC_USE_DBUS
    -DIPC_USE_UNIX_SOCKET
    -DIPC_USE_SHARED_MEMORY
)

target_link_libraries(ipc
    ${JSONCPP_LIBRARIES}
    pthread
    rt
)

# Conditionally link DBus
find_package(DBus REQUIRED)
if(DBus_FOUND)
    target_compile_definitions(ipc PRIVATE -DIPC_HAVE_DBUS)
    target_link_libraries(ipc ${DBus_LIBRARIES})
endif()

# Install
install(TARGETS ipc DESTINATION lib)
install(FILES ${IPC_HEADERS} DESTINATION include/ipc)

# Tests
option(BUILD_IPC_TESTS "Build IPC tests" ON)
if(BUILD_IPC_TESTS)
    find_package(GTest REQUIRED)
    add_executable(ipc_test
        tests/IpcTest.cpp
        tests/IpcMessageTest.cpp
        tests/IpcClientServerTest.cpp
    )
    target_link_libraries(ipc_test
        ipc
        GTest::GTest
        GTest::Main
    )
    target_include_directories(ipc_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME ipc_test COMMAND ipc_test)
endif()
