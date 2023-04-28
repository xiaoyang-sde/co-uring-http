#ifndef IO_URING_HPP
#define IO_URING_HPP

#include <coroutine>
#include <functional>
#include <liburing.h>

namespace co_uring_http {
struct sqe_user_data {
  enum type {
    ACCEPT,
    RECV,
    SEND,
    READ,
    WRITE,
  };

  type type;
  void *coroutine;
  int result;
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

  auto for_each_cqe(std::function<void(io_uring_cqe *)> functor) -> void;

  auto cqe_seen(io_uring_cqe *cqe) -> void;

  auto submit_and_wait(const int wait_nr) -> int;

  auto submit_multishot_accept_request(
      int fd, sqe_user_data *sqe_data, sockaddr *client_addr,
      socklen_t *client_len
  ) -> void;

  auto submit_recv_request(
      int fd, sqe_user_data *sqe_data, std::vector<char> &buffer
  ) -> void;

  auto submit_send_request(
      int fd, sqe_user_data *sqe_data, const std::vector<char> &buffer
  ) -> void;

  auto submit_cancel_request(sqe_user_data *sqe_data) -> void;

  io_uring ring;
};
} // namespace co_uring_http

#endif
