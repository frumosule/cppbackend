cmake_minimum_required(VERSION 3.11)

project(cafeteria CXX)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
  include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

find_package(Boost 1.84.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(cafeteria
	src/main.cpp
	src/cafeteria.h
	src/result.h
	src/hotdog.h
	src/gascooker.h
	src/ingredients.h
	src/clock.h
)
target_link_libraries(cafeteria PRIVATE Threads::Threads Boost::system)

if(WIN32)
target_link_libraries(cafeteria PRIVATE ws2_32 mswsock)
endif()
