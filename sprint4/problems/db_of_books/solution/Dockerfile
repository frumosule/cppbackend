FROM gcc:11.3

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11
COPY CMakeLists.txt /app/
COPY main.cpp /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

RUN groupadd -r www && useradd -r -g www www
USER www

ENTRYPOINT ["/app/build/book_manager", "postgres://postgres:Mys3Cr3t@172.17.0.2:5432/postgres"]