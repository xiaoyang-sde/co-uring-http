export module co_uring_http.io:constant;

import std;

namespace co_uring_http {

export constexpr std::uint_least16_t PORT = 8080;

export constexpr std::uint_least32_t IO_URING_QUEUE_SIZE = 2048;

export constexpr std::uint_least32_t SOCKET_LISTEN_QUEUE_SIZE = 512;

export constexpr std::uint_least32_t BUF_GROUP_ID = 0;

export constexpr std::uint_least32_t BUF_RING_SIZE = 4096;

export constexpr std::uint_least32_t BUF_SIZE = 1024;

} // namespace co_uring_http
