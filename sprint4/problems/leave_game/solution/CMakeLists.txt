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

find_package(Catch2 REQUIRED)

# ИСПРАВЛЕНО: Явное указание путей к PostgreSQL
set(POSTGRESQL_ROOT "C:/Program Files/PostgreSQL/18")
set(POSTGRESQL_INCLUDE_DIR "${POSTGRESQL_ROOT}/include")
set(POSTGRESQL_LIBRARY "${POSTGRESQL_ROOT}/lib/libpq.lib")

# Проверяем существование файлов
if(EXISTS "${POSTGRESQL_INCLUDE_DIR}/libpq-fe.h" AND EXISTS "${POSTGRESQL_LIBRARY}")
    message(STATUS "Found PostgreSQL include: ${POSTGRESQL_INCLUDE_DIR}")
    message(STATUS "Found PostgreSQL library: ${POSTGRESQL_LIBRARY}")
    
    # Создаем импортированную цель для PostgreSQL
    add_library(PostgreSQL::PostgreSQL INTERFACE IMPORTED)
    set_target_properties(PostgreSQL::PostgreSQL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${POSTGRESQL_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${POSTGRESQL_LIBRARY}"
    )
    
    set(PostgreSQL_FOUND TRUE)
else()
    message(FATAL_ERROR "PostgreSQL not found! Checked at: ${POSTGRESQL_ROOT}")
endif()

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
    src/collision_handler.cpp
    src/collision_handler.h
    src/collision_detector.cpp
    src/collision_detector.h
    src/geom.h
    src/player.cpp
    src/serializing_listener.h
    src/serializing_listener.cpp
    src/application_listener.h
    src/ticker.h
    src/ticker.cpp
    src/retired_players_manager.cpp
    src/retired_players_manager.h
)

# Указываем include директории
target_include_directories(game_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${POSTGRESQL_INCLUDE_DIR}  # Добавляем PostgreSQL include
)

target_link_libraries(game_server PRIVATE
    Threads::Threads
    Boost::system
    Boost::json
    Boost::log
    Boost::program_options
    Boost::date_time
    PostgreSQL::PostgreSQL
)

if(WIN32)
    target_link_libraries(game_server PRIVATE 
        ws2_32 
        mswsock 
    )
endif()

# Простые тесты без внешних зависимостей
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/state-serialization-tests.cpp)
    add_executable(loot_generator_test
        tests/state-serialization-tests.cpp
    )
    
    target_include_directories(loot_generator_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${POSTGRESQL_INCLUDE_DIR}  # Добавляем PostgreSQL include для тестов
    )
    
    target_link_libraries(loot_generator_test PRIVATE
        Threads::Threads
        Boost::system
        Boost::json
        Boost::log
        Boost::program_options
        Boost::date_time
        Catch2::Catch2WithMain 
        PostgreSQL::PostgreSQL
    )
    
    add_custom_target(run_tests
        COMMAND loot_generator_test
        DEPENDS loot_generator_test
        COMMENT "Running loot generator tests..."
    )

endif()



