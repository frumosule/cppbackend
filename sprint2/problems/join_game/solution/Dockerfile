FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing

COPY ./src /app/src
COPY CMakeLists.txt /app/
COPY ./static /app/static

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

FROM ubuntu:22.04 as run

RUN groupadd -r www && useradd -r -g www www
USER www

# Копируем исполняемый файл (проверьте правильный путь)
COPY --from=build /app/build/bin/game_server /app/
# ИЛИ если в корне build:
# COPY --from=build /app/build/game_server /app/

# Копируем данные и статику
COPY ./data /app/data
COPY --from=build /app/static /app/static

# Исправленный entrypoint с тремя аргументами
ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static"]