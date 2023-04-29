#ifndef CONSTANT_HPP
#define CONSTANT_HPP

#include <cstddef>

namespace co_uring_http {

constexpr unsigned int SOCKET_LISTEN_QUEUE_SIZE = 512;

constexpr unsigned int MAX_BUFFER_RING_SIZE = 65536;

constexpr size_t IO_URING_QUEUE_SIZE = 2048;

constexpr unsigned int BUFFER_GROUP_ID = 0;

constexpr unsigned int BUFFER_RING_SIZE = 1024;

constexpr size_t BUFFER_SIZE = 1024;

} // namespace co_uring_http

#endif
