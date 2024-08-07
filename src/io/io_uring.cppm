module;

#include <fcntl.h>
#include <liburing.h>

export module co_uring_http.io:io_uring;

import std;

import co_uring_http.coroutine;

import :constant;
import :error;

namespace co_uring_http {

export class sqe_data {
public:
  void *coroutine = nullptr;
  std::int_least32_t cqe_res = 0;
  std::uint_least32_t cqe_flags = 0;
};

export class io_uring_context {
public:
  [[nodiscard]] static auto get_instance() noexcept -> io_uring_context & {
    thread_local io_uring_context instance;
    return instance;
  }

  ~io_uring_context() noexcept { io_uring_queue_exit(&io_uring_); }

  [[nodiscard]] auto
  queue_init() noexcept -> std::expected<void, std::uint_least16_t> {
    return decode_void(io_uring_queue_init(IO_URING_QUEUE_SIZE, &io_uring_, 0));
  }

  auto event_loop() noexcept -> void {
    while (true) {
      unwrap(submit_and_wait(1));

      std::uint_least32_t head = 0;
      io_uring_cqe *cqe = nullptr;
      io_uring_for_each_cqe(&io_uring_, head, cqe) {

        auto *const sqe_data =
            reinterpret_cast<class sqe_data *>(io_uring_cqe_get_data(cqe));

        sqe_data->cqe_res = cqe->res;
        sqe_data->cqe_flags = cqe->flags;
        void *const coroutine_address = sqe_data->coroutine;
        io_uring_cqe_seen(&io_uring_, cqe);

        if (coroutine_address != nullptr) {
          std::coroutine_handle<>::from_address(coroutine_address).resume();
        }
      }
    }
  }

  [[nodiscard]] auto submit_and_wait(const std::uint_least32_t wait_nr) noexcept
      -> std::expected<std::uint_least32_t, std::uint_least16_t> {
    return decode(io_uring_submit_and_wait(&io_uring_, wait_nr));
  }

  auto submit_multishot_accept_request(
      sqe_data *const sqe_data, const std::uint_least32_t raw_fd,
      sockaddr *const client_addr,
      socklen_t *const client_len) noexcept -> void {
    io_uring_sqe *const sqe = io_uring_get_sqe(&io_uring_);
    io_uring_prep_multishot_accept(sqe, raw_fd, client_addr, client_len, 0);
    io_uring_sqe_set_data(sqe, sqe_data);
  }

  auto submit_recv_request(sqe_data *const sqe_data,
                           const std::uint_least32_t raw_fd) noexcept -> void {
    io_uring_sqe *const sqe = io_uring_get_sqe(&io_uring_);
    io_uring_prep_recv(sqe, raw_fd, nullptr, BUF_SIZE, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
    io_uring_sqe_set_data(sqe, sqe_data);
    sqe->buf_group = BUF_GROUP_ID;
  }

  auto submit_send_request(
      sqe_data *const sqe_data, const std::uint_least32_t raw_fd,
      const std::span<const std::uint_least8_t> buf) noexcept -> void {
    io_uring_sqe *const sqe = io_uring_get_sqe(&io_uring_);
    io_uring_prep_send(sqe, raw_fd, buf.data(), buf.size(), 0);
    io_uring_sqe_set_data(sqe, sqe_data);
  }

  auto submit_splice_request(sqe_data *const sqe_data,
                             const std::uint_least32_t raw_fd_in,
                             const std::uint_least32_t raw_fd_out,
                             const std::uint_least32_t len) noexcept -> void {
    io_uring_sqe *const sqe = io_uring_get_sqe(&io_uring_);
    io_uring_prep_splice(sqe, raw_fd_in, -1, raw_fd_out, -1, len,
                         SPLICE_F_MORE);
    io_uring_sqe_set_data(sqe, sqe_data);
  }

  auto submit_cancel_request(sqe_data *const sqe_data) noexcept -> void {
    io_uring_sqe *const sqe = io_uring_get_sqe(&io_uring_);
    io_uring_prep_cancel(sqe, sqe_data, 0);
  }

  [[nodiscard]] auto setup_buf_ring(
      io_uring_buf_ring *const buf_ring,
      const std::span<std::vector<std::uint_least8_t>> buf_list) noexcept
      -> std::expected<void, std::uint_least16_t> {
    io_uring_buf_reg io_uring_buf_reg{
        .ring_addr = reinterpret_cast<std::uint_least64_t>(buf_ring),
        .ring_entries = static_cast<std::uint_least32_t>(BUF_RING_SIZE),
        .bgid = BUF_GROUP_ID,
    };

    return decode_void(
               io_uring_register_buf_ring(&io_uring_, &io_uring_buf_reg, 0))
        .transform([buf_ring, buf_list]() noexcept -> void {
          io_uring_buf_ring_init(buf_ring);
          const std::uint_least32_t mask =
              io_uring_buf_ring_mask(BUF_RING_SIZE);
          for (const auto buf_id :
               std::views::iota(0) | std::views::take(BUF_RING_SIZE)) {
            io_uring_buf_ring_add(buf_ring, buf_list[buf_id].data(),
                                  buf_list[buf_id].size(), buf_id, mask,
                                  buf_id);
          }
          io_uring_buf_ring_advance(buf_ring, BUF_RING_SIZE);
        });
  };

  auto add_buf(io_uring_buf_ring *const buf_ring,
               const std::span<std::uint_least8_t> buf,
               const std::uint_least32_t buf_id) noexcept -> void {
    const std::uint_least32_t mask = io_uring_buf_ring_mask(BUF_RING_SIZE);
    io_uring_buf_ring_add(buf_ring, buf.data(), buf.size(), buf_id, mask,
                          buf_id);
    io_uring_buf_ring_advance(buf_ring, 1);
  }

private:
  io_uring io_uring_;
};

}; // namespace co_uring_http
