cmake_minimum_required(VERSION 3.10)
project(game_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")

# Поиск исходных файлов
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

# Создание исполняемого файла сервера
add_executable(server ${SOURCES} ${HEADERS})

# Целевые директории для установки
install(TARGETS server DESTINATION bin)

# Копирование скриптов и конфигурационных файлов
install(FILES shoot.py DESTINATION scripts)
install(FILES config.json DESTINATION config)
install(DIRECTORY FlameGraph DESTINATION scripts)