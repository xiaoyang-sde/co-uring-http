#ifndef IO_URING_HPP
#define IO_URING_HPP

#include <coroutine>
#include <functional>
#include <liburing.h>
#include <span>

namespace co_uring_http {
struct sqe_data {
  void *coroutine;
  int cqe_res;
  unsigned int cqe_flags;
};

class io_uring_handler {
public:
  static auto get_instance() noexcept -> io_uring_handler &;

  io_uring_handler();

  ~io_uring_handler();

  io_uring_handler(io_uring_handler &&other) = delete;

  auto operator=(io_uring_handler &&other) -> io_uring_handler & = delete;

  io_uring_handler(const io_uring_handler &other) = delete;

  auto operator=(const io_uring_handler &other) -> io_uring_handler & = delete;

  auto get_uring() noexcept -> io_uring &;

  auto for_each_cqe(const std::function<void(io_uring_cqe *)> &lambda) -> void;

  auto cqe_seen(io_uring_cqe *cqe) -> void;

  auto submit_and_wait(int wait_nr) -> int;

  auto submit_multishot_accept_request(
      int raw_file_descriptor, sqe_data *sqe_data, sockaddr *client_addr,
      socklen_t *client_len
  ) -> void;

  auto submit_recv_request(
      int raw_file_descriptor, sqe_data *sqe_data, size_t length
  ) -> void;

  auto submit_send_request(
      int raw_file_descriptor, sqe_data *sqe_data,
      const std::span<std::byte> &buffer, size_t length
  ) -> void;

  auto submit_cancel_request(sqe_data *sqe_data) -> void;

  auto setup_buffer_ring(
      io_uring_buf_ring *buffer_ring,
      std::span<std::vector<std::byte>> buffer_list,
      unsigned int buffer_ring_size
  ) -> void;

  auto add_buffer(
      io_uring_buf_ring *buffer_ring, std::span<std::byte> buffer,
      unsigned int buffer_id, unsigned int buffer_ring_size
  ) -> void;

private:
  io_uring io_uring_;
};
} // namespace co_uring_http

#endif
