FROM ubuntu:24.04 as base

RUN apt update -y && \
    apt install -y \
    git \
    wget \
    g++ \
    cmake \
    ninja-build \
    libssl-dev \
    libpq-dev

# Install boost
WORKDIR /root
RUN wget https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz && \
    tar -xf boost_1_85_0.tar.gz
WORKDIR /root/boost_1_85_0
RUN ./bootstrap.sh --with-libraries=system,thread && \
    ./b2 install


FROM base as build

WORKDIR /app
COPY . .

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTOWER_BUILD_TESTS=OFF -DTOWER_BUILD_DUMMY=OFF && \
    cmake --build build --config Release --target tower-server

CMD ["/app/build/tower-server"]