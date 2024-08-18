FROM ubuntu:24.04 as base

RUN apt update -y && \
    apt install -y \
    git \
    wget \
    g++ \
    cmake \
    ninja-build \
    python3-pip \
    python3-dev \
    libssl-dev \
    libpq-dev

# Install boost and python dependencies
WORKDIR /root
RUN wget https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz && \
    tar -xf boost_1_85_0.tar.gz
WORKDIR /root/boost_1_85_0
RUN ./bootstrap.sh --with-python=$(which python3) --with-libraries=python,system,thread && \
    ./b2 install

WORKDIR /app
COPY . .
RUN pip install --no-cache-dir --break-system-packages -r requirements.txt


FROM base as build

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTOWER_BUILD_TESTS=OFF && \
    cmake --build build --config Release --target tower-server

CMD ["/app/build/tower-server"]