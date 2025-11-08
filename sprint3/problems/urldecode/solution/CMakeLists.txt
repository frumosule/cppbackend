cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Включаем Conan toolchain если существует
if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

# Ищем Boost с компонентами
find_package(Boost 1.84.0 REQUIRED COMPONENTS unit_test_framework)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Boost found: ${Boost_VERSION}")
    message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Основное приложение
add_executable(urldecode
    src/main.cpp
    src/urldecode.h
    src/urldecode.cpp
)

# Тесты
add_executable(tests
    tests/tests.cpp
    src/urldecode.h
    src/urldecode.cpp
)

# Правильное линкование для тестов
if(Boost_FOUND)
    target_link_libraries(tests PRIVATE 
        Boost::unit_test_framework
        Threads::Threads
    )
else()
    # Альтернативный вариант если CONAN_PKG не работает
    target_link_libraries(tests PRIVATE 
        ${Boost_LIBRARIES}
        Threads::Threads
    )
endif()

if(WIN32)
    target_link_libraries(urldecode PRIVATE ws2_32 mswsock)
    target_link_libraries(tests PRIVATE ws2_32 mswsock)
endif()