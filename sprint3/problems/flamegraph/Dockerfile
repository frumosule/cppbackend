FROM ubuntu:20.04

# Установка необходимых пакетов
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    python3 \
    python3-pip \
    curl \
    linux-tools-common \
    linux-tools-generic \
    perl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Клонирование FlameGraph репозитория
RUN git clone https://github.com/brendangregg/FlameGraph.git

# Установка рабочей директории
WORKDIR /app

# Копирование исходных файлов
COPY . .

# Установка прав на выполнение для скриптов FlameGraph
RUN chmod +x FlameGraph/*.pl

# Установка Python зависимостей (если нужны)
RUN pip3 install requests

# Команда по умолчанию
CMD ["python3", "shoot.py", "./server config.json"]