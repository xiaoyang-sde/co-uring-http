#ifndef BUFFER_RING_HPP
#define BUFFER_RING_HPP

#include <bitset>
#include <liburing.h>
#include <memory>
#include <optional>
#include <span>

#include "constant.hpp"
#include "io_uring.hpp"

namespace co_uring_http {
class buffer_ring {
public:
  static auto get_instance() noexcept -> buffer_ring & {
    thread_local buffer_ring instance;
    return instance;
  }

  auto register_buffer_ring(
      const unsigned int buffer_ring_size, const size_t buffer_size
  ) -> void {
    const size_t ring_entries_size = buffer_ring_size * sizeof(io_uring_buf);
    const size_t page_size = sysconf(_SC_PAGESIZE);
    void *buffer_ring = nullptr;
    posix_memalign(&buffer_ring, page_size, ring_entries_size);
    buffer_ring_.reset(reinterpret_cast<io_uring_buf_ring *>(buffer_ring));

    buffer_list_.reserve(buffer_ring_size);
    for (unsigned int i = 0; i < buffer_ring_size; ++i) {
      buffer_list_.emplace_back(buffer_size);
    }

    io_uring_handler::get_instance().setup_buffer_ring(
        buffer_ring_.get(), buffer_list_, buffer_list_.size()
    );
  }

  auto borrow_buffer(const unsigned int buffer_id, const size_t size)
      -> std::span<std::byte> {
    borrowed_buffer_set_[buffer_id] = true;
    return std::span(buffer_list_[buffer_id].data(), size);
  }

  auto return_buffer(const unsigned int buffer_id) -> void {
    borrowed_buffer_set_[buffer_id] = false;
    io_uring_handler::get_instance().add_buffer(
        buffer_ring_.get(), buffer_list_[buffer_id], buffer_id,
        buffer_list_.size()
    );
  }

private:
  std::unique_ptr<io_uring_buf_ring> buffer_ring_;
  std::vector<std::vector<std::byte>> buffer_list_;
  std::bitset<MAX_BUFFER_RING_SIZE> borrowed_buffer_set_;
};
} // namespace co_uring_http

#endif
