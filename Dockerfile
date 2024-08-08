FROM alpine:latest

RUN apk add --no-cache \
    git \
    g++ \
    cmake \
    ninja \
    boost-dev \
    libpq-dev

WORKDIR /app
COPY . .

RUN cmake -B build -G Ninja -DTOWER_BUILD_TESTS=OFF -DFLATBUFFERS_BUILD_TESTS=OFF && \
    cmake --build build --config Release

EXPOSE 30000

ENTRYPOINT ["/app/build/tower-server"]