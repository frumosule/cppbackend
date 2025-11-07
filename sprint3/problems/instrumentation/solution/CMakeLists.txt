cmake_minimum_required(VERSION 3.10)
project(event2dot_v2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "*.cpp")

add_executable(event2dot_v2 ${SOURCES})
