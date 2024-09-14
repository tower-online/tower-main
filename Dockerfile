FROM ubuntu:24.04 AS base

RUN apt-get update && \
    apt-get install -y \
    git \
    wget \
    g++ \
    cmake \
    ninja-build \
    libssl-dev \
    # bpftool dependencies
    libelf-dev \
    clang \
    llvm \
    pkg-config

# Install boost
WORKDIR /root
RUN wget https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz && \
    tar -xf boost_1_85_0.tar.gz
WORKDIR /root/boost_1_85_0
RUN ./bootstrap.sh --with-libraries=charconv,system && \
    ./b2 install

WORKDIR /app
COPY . .

# Install bpftool
WORKDIR /app/plugins/tracer/bpftool/src
RUN make install


FROM base AS build

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTOWER_BUILD_TESTS=OFF && \
    cmake --build build --config Release --target tower-schema-packet tower-schema-world tower-server

CMD ["/app/build/tower-server"]