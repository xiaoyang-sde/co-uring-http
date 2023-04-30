FROM ubuntu:lunar

RUN apt update
RUN apt install -y \
    build-essential \
    clang \
    clang-format \
    clang-tidy \
    clangd \
    cmake \
    git \
    make \
    python3 \
    python3-pip

RUN git clone https://github.com/axboe/liburing.git \
    && make -C liburing install \
    && rm -r liburing
