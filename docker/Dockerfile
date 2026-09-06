FROM ubuntu:26.04 AS dev

ARG LLVM_VERSION=23.1.0
ARG CMAKE_VERSION=4.4.2
ARG NINJA_VERSION=1.13.2
ARG LIBURING_VERSION=2.15

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
        build-essential \
        ca-certificates \
        curl \
        pkg-config \
        sudo \
        unzip \
        xz-utils \
        libc++-dev \
        libc++abi-dev \
    && rm -rf /var/lib/apt/lists/*

RUN curl --fail --silent --show-error --location \
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz" \
        --output /tmp/llvm.tar.xz \
    && mkdir --parents /opt/llvm \
    && tar --extract --xz --strip-components=1 --file /tmp/llvm.tar.xz --directory /opt/llvm \
    && ln --symbolic /opt/llvm/bin/clang /usr/local/bin/clang \
    && ln --symbolic /opt/llvm/bin/clang++ /usr/local/bin/clang++ \
    && ln --symbolic /opt/llvm/bin/clang-scan-deps /usr/local/bin/clang-scan-deps \
    && ln --symbolic /opt/llvm/bin/clangd /usr/local/bin/clangd \
    && curl --fail --silent --show-error --location \
        "https://raw.githubusercontent.com/llvm/llvm-project/llvmorg-${LLVM_VERSION}/libcxx/include/text_encoding" \
        --output /opt/llvm/share/libc++/v1/text_encoding \
    && rm /tmp/llvm.tar.xz

RUN curl --fail --silent --show-error --location \
        "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh" \
        --output /tmp/cmake.sh \
    && chmod +x /tmp/cmake.sh \
    && mkdir --parents /opt/cmake \
    && /tmp/cmake.sh --skip-license --prefix=/opt/cmake \
    && rm /tmp/cmake.sh

RUN curl --fail --silent --show-error --location \
        "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip" \
        --output /tmp/ninja.zip \
    && unzip /tmp/ninja.zip -d /usr/local/bin \
    && ln --symbolic /usr/local/bin/ninja /usr/bin/ninja-build \
    && rm /tmp/ninja.zip

RUN mkdir /tmp/liburing \
    && curl --fail --silent --show-error --location \
        "https://github.com/axboe/liburing/archive/refs/tags/liburing-${LIBURING_VERSION}.tar.gz" \
        --output /tmp/liburing.tar.gz \
    && tar --extract --gzip --strip-components=1 --file /tmp/liburing.tar.gz --directory /tmp/liburing \
    && cd /tmp/liburing \
    && ./configure --prefix=/usr/local \
    && make --jobs="$(nproc)" \
    && make install \
    && cd / \
    && rm -rf /tmp/liburing /tmp/liburing.tar.gz \
    && ldconfig

ENV PATH="/opt/cmake/bin:${PATH}" \
    CC=clang \
    CXX=clang++ \
    CXXFLAGS=-stdlib=libc++ \
    CMAKE_GENERATOR=Ninja

WORKDIR /workspace

CMD ["bash"]
