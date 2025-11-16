# Dockerfile для сборки и запуска тестов collision_detection_tests

# Этап сборки
FROM gcc:11.3 AS build

# Устанавливаем системные зависимости и Conan
RUN apt update && \
    apt install -y python3-pip cmake && \
    pip3 install "conan>=1,<2"

# Рабочая директория внутри контейнера
WORKDIR /app

# Копируем файл описания зависимостей Conan и ставим их
COPY conanfile.txt ./
RUN mkdir build && cd build && \
    conan install .. --build=missing \
      -s compiler.libcxx=libstdc++11 \
      -s build_type=Release

# Копируем исходники и тесты
COPY src    ./src
COPY tests  ./tests
COPY CMakeLists.txt ./

# Собираем проект в режиме Release
RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# По умолчанию при запуске контейнера — запуск тестов
ENTRYPOINT ["./build/collision_detection_tests"]