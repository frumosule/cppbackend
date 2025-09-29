cmake_minimum_required(VERSION 3.11)

# ������ ���������� Hello � ������� �� C++
project(Hello CXX)
# ������� ��� ����� ��������������� � ���������� ��������� �++ 20
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
        include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    endif()

# ���������� ��������������� ������ conanbuildinfo.cmake, ��������� Conan

# ���� Boost ������ 1.78
find_package(Boost 1.84.0 REQUIRED)
if(Boost_FOUND)
  # boost ������, ��������� � ��������� ������������ ������ ������� ���� �
  # ������������ ������ boost
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# ��������� ����� linux ������� ����������� ���������� pthread ���
# ��������� ����������� �������.
# ��������� ��� ������ ��������� ��� ���������� �� ����� ����������
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ������ �������� ������������ �������� ���� src/main.cpp
add_executable(hello src/main.cpp)

# ������ ����������� ���������� ���������� ��� ��������� �������
target_link_libraries(hello PRIVATE Threads::Threads Boost::system)
if(WIN32)
target_link_libraries(hello PRIVATE ws2_32 mswsock)
endif()
