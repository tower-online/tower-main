FROM alpine:latest

RUN apk add --no-cache \
    git \
    g++ \
    cmake \
    ninja \
    boost-dev \
    libpq-dev \
    postgresql-client

WORKDIR /app
COPY . .

EXPOSE 30000

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTOWER_BUILD_TESTS=OFF && \
    cmake --build build --config Release --target tower-server

ENTRYPOINT ["/app/build/tower-server"]