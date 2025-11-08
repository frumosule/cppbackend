# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
      git \
      linux-tools-common \
      linux-tools-generic \
      perl \
    && \
    pip3 install conan==1.*

# Клонирование FlameGraph в корень
RUN git clone https://github.com/brendangregg/FlameGraph.git

# Создаем рабочую директорию
WORKDIR /app

# Запуск conan как раньше
COPY conanfile.txt ./
RUN mkdir build && cd build && \
    conan install .. --build=missing

# Копируем исходники
COPY ./src ./src
COPY CMakeLists.txt ./

RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Устанавливаем необходимые пакеты для perf и скриптов
RUN apt update && apt install -y \
    linux-tools-common \
    linux-tools-generic \
    perl \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

WORKDIR /app

# Копируем FlameGraph скрипты из build-контейнера
COPY --from=build /FlameGraph ./FlameGraph

# Копируем приложение и данные
COPY --from=build /app/build/bin/game_server ./
COPY ./data ./data

# Даем права на выполнение
RUN chmod +x ./FlameGraph/*.pl

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "/app/data/config.json"]