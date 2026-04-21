FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    cmake \
    make \
    libboost-all-dev \
    nlohmann-json3-dev \
    libssl-dev \
    libyaml-cpp-dev \
    ca-certificates \
    gdb \
    rsync \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
