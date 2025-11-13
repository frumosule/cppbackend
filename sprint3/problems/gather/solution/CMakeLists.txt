cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
  include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

find_package(Boost 1.84.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем пути для include файлов
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Ищем Catch2
find_package(Catch2 REQUIRED)

add_library(collision_detection_lib STATIC
	src/collision_detector.cpp  # Убрал .h файлы
)

target_link_libraries(collision_detection_lib PUBLIC 
    Threads::Threads
	Boost::system
)

add_executable(collision_detection_tests
	tests/collision-detector-tests.cpp
)

target_link_libraries(collision_detection_tests 
    collision_detection_lib 
    Catch2::Catch2WithMain
    Threads::Threads
    ${Boost_LIBRARIES}
)
