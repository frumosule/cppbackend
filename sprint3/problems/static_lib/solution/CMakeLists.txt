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

find_package(Boost 1.84.0 REQUIRED
    COMPONENTS
        system
        json
        log
        date_time
        program_options
)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Основной исполняемый файл сервера
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
    src/player.h
    src/game_session_manager.h
    src/game_session_manager.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/logging/formatter.h
    src/logging/formatter.cpp
    src/logging/logger.h
    src/logging/logger.cpp
    src/logging/logging_handler.h
)

# Указываем include директории
target_include_directories(game_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_server PRIVATE
    Threads::Threads
    Boost::system
    Boost::json
    Boost::log
    Boost::program_options
    Boost::date_time
)

if(WIN32)
    target_link_libraries(game_server PRIVATE ws2_32 mswsock)
endif()

# Добавляем флаги компиляции для лучшей отладки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(game_server PRIVATE BOOST_LOG_DYN_LINK)
    target_compile_options(game_server PRIVATE -g -O0)
else()
    target_compile_options(game_server PRIVATE -O2 -DNDEBUG)
endif()

# Предупреждения компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(game_server PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Простые тесты без внешних зависимостей
# ИСПРАВЛЕНО: правильное имя файла
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/loot_generator_tests.cpp)
    add_executable(loot_generator_test
        src/loot_generator.cpp
        tests/loot_generator_tests.cpp
        src/model.cpp
        src/game_session_manager.cpp
        src/json_loader.cpp
    )
    
    target_include_directories(loot_generator_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    
    target_link_libraries(loot_generator_test PRIVATE
        Threads::Threads
        Boost::system
        Boost::json
        Boost::log
        Boost::program_options
        Boost::date_time
    )
    
    add_custom_target(run_tests
        COMMAND loot_generator_test
        DEPENDS loot_generator_test
        COMMENT "Running loot generator tests..."
    )
endif()