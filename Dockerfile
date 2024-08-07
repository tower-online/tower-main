FROM alpine:latest

RUN apk update && apk add --no-cache \
    git \
    g++ \
    cmake \
    ninja \
    boost-dev

RUN mkdir /app
WORKDIR /app
COPY . /app

RUN cmake -B build -G Ninja -DTOWER_BUILD_TESTS=OFF && \
    cmake --build build --config Release

EXPOSE 30000

ENTRYPOINT ["/app/build/tower-server"]