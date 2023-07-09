FROM ubuntu:lunar

RUN apt update
RUN apt install -y \
    clang-16 \
    clang-format-16 \
    clang-tidy-16 \
    clangd-16 \
    cmake \
    git \
    g++-13 \
    gdb \
    libclang-16-dev \
    libstdc++-13-dev \
    make \
    python3 \
    python3-pip

RUN git clone https://github.com/axboe/liburing.git \
    && make -C liburing install \
    && rm -r liburing
