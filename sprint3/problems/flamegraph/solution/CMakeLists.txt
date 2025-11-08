cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
  include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

include_directories(${Boost_INCLUDE_DIRS})


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
        src/main.cpp
        src/http_server.cpp
        src/http_server.h
        src/sdk.h
        src/model.h
        src/model.cpp
        src/tagged.h
        src/json_loader.h
        src/json_loader.cpp
        src/request_handler.cpp
        src/request_handler.h
)

target_link_libraries(game_server PRIVATE Threads::Threads Boost::system Boost::json)

if(WIN32)
target_link_libraries(game_server PRIVATE ws2_32 mswsock)
endif()