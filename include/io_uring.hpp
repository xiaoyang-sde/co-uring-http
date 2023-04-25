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
  size_t result;
  int fd;
};

class io_uring_handler {
public:
  explicit io_uring_handler(const size_t queue_size);

  ~io_uring_handler();

  io_uring &get_uring() noexcept;

  auto for_each_cqe(std::function<void(io_uring_cqe *)> functor) -> void;

  auto cqe_seen(io_uring_cqe *cqe) -> void;

  auto submit_and_wait(const int wait_nr) -> int;

  auto submit_accept_request(int fd, sqe_user_data *sqe_data,
                             sockaddr *client_addr, socklen_t *client_len)
      -> void;

  auto submit_recv_request(int fd, sqe_user_data *sqe_data,
                           std::vector<char> &buffer) -> void;

  auto submit_send_request(int fd, sqe_user_data *sqe_data,
                           const std::vector<char> &buffer) -> void;

  io_uring ring;
};

class recv_awaitable {
public:
  recv_awaitable(io_uring_handler &io_uring_handler, const int fd,
                 std::vector<char> &buffer);

  bool await_ready();
  void await_suspend(std::coroutine_handle<>);
  size_t await_resume();

private:
  int fd;
  io_uring_handler &io_uring_handler;
  std::vector<char> &buffer;
  sqe_user_data sqe_user_data;
};

class send_awaitable {
public:
  send_awaitable(io_uring_handler &io_uring_handler, const int fd,
                 const std::vector<char> &buffer);

  bool await_ready();
  void await_suspend(std::coroutine_handle<>);
  size_t await_resume();

private:
  int fd;
  io_uring_handler &io_uring_handler;
  const std::vector<char> &buffer;
  sqe_user_data sqe_user_data;
};

class accept_awaitable {
public:
  accept_awaitable(io_uring_handler &io_uring_handler, const int fd,
                   sockaddr_storage *client_address,
                   socklen_t *client_address_size);

  bool await_ready();
  void await_suspend(std::coroutine_handle<>);
  size_t await_resume();

private:
  int fd;
  io_uring_handler &io_uring_handler;
  sqe_user_data sqe_user_data;
  sockaddr_storage *client_address;
  socklen_t *client_address_size;
};
} // namespace co_uring_http

#endif
