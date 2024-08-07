module;

#include <liburing.h>
#include <stdlib.h>
#include <unistd.h>

export module co_uring_http.io:buf_ring;

import std;

import :constant;
import :io_uring;
import :error;

namespace co_uring_http {

export class buf_ring {
public:
  [[nodiscard]] static auto get_instance() noexcept -> buf_ring & {
    thread_local buf_ring instance;
    return instance;
  }

  [[nodiscard]] auto
  register_buf_ring() noexcept -> std::expected<void, std::uint_least16_t> {
    void *buf_ring = nullptr;
    return decode_void(posix_memalign(&buf_ring, sysconf(_SC_PAGESIZE),
                                      BUF_RING_SIZE * sizeof(io_uring_buf)))
        .and_then(
            [this,
             buf_ring]() noexcept -> std::expected<void, std::uint_least16_t> {
              buf_ring_.reset(reinterpret_cast<io_uring_buf_ring *>(buf_ring));

              buf_list_.reserve(BUF_RING_SIZE);
              for (const auto _ :
                   std::views::iota(0) | std::views::take(BUF_RING_SIZE)) {
                buf_list_.emplace_back(BUF_SIZE);
              }

              return io_uring_context::get_instance().setup_buf_ring(
                  buf_ring_.get(), buf_list_);
            });
  }

  [[nodiscard]] auto borrow_buf(const std::uint_least32_t buf_id) noexcept
      -> std::span<std::uint_least8_t> {
    borrowed_buf_set_[buf_id] = true;
    return buf_list_[buf_id];
  }

  auto return_buf(const std::uint_least32_t buf_id) noexcept -> void {
    borrowed_buf_set_[buf_id] = false;
    io_uring_context::get_instance().add_buf(buf_ring_.get(), buf_list_[buf_id],
                                             buf_id);
  }

private:
  std::bitset<BUF_RING_SIZE> borrowed_buf_set_;
  std::vector<std::vector<std::uint_least8_t>> buf_list_;
  std::unique_ptr<io_uring_buf_ring> buf_ring_;
};

} // namespace co_uring_http
