#ifndef BUFFER_RING_HPP
#define BUFFER_RING_HPP

#include <liburing/io_uring.h>

#include <bitset>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "constant.hpp"

namespace co_uring_http {
class buffer_ring {
public:
  static auto get_instance() noexcept -> buffer_ring &;

  auto register_buffer_ring(const unsigned int buffer_ring_size, const size_t buffer_size) -> void;
  auto borrow_buffer(const unsigned int buffer_id, const size_t size) -> std::span<std::byte>;
  auto return_buffer(const unsigned int buffer_id) -> void;

private:
  std::unique_ptr<io_uring_buf_ring> buffer_ring_;
  std::vector<std::vector<std::byte>> buffer_list_;
  std::bitset<MAX_BUFFER_RING_SIZE> borrowed_buffer_set_;
};
} // namespace co_uring_http

#endif
