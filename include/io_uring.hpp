#ifndef IO_URING_HPP
#define IO_URING_HPP

#include <coroutine>
#include <functional>
#include <iostream>
#include <liburing.h>
#include <stdexcept>
#include <vector>

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
  explicit io_uring_handler(const size_t queue_size) {
    if (const int result = io_uring_queue_init(queue_size, &ring, 0);
        result != 0) {
      throw std::runtime_error("failed to invoke 'io_uring_queue_init'");
    }
  }

  ~io_uring_handler() { io_uring_queue_exit(&ring); }

  io_uring &get_uring() noexcept { return ring; }

  auto for_each_cqe(std::function<void(io_uring_cqe *)> functor) -> void {
    io_uring_cqe *cqe;
    unsigned int head;

    io_uring_for_each_cqe(&ring, head, cqe) { functor(cqe); }
  }

  auto cqe_seen(io_uring_cqe *cqe) -> void { io_uring_cqe_seen(&ring, cqe); }

  auto submit_and_wait(const int wait_nr) -> int {
    const int result = io_uring_submit_and_wait(&ring, wait_nr);
    if (result < 0) {
      throw std::runtime_error("failed to invoke 'io_uring_submit_and_wait'");
    }
    return result;
  }

  auto submit_accept_request(int fd, sqe_user_data *sqe_data,
                             sockaddr *client_addr, socklen_t *client_len)
      -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_accept(sqe, fd, client_addr, client_len, 0);
    io_uring_sqe_set_data(sqe, sqe_data);
  }

  auto submit_recv_request(int fd, sqe_user_data *sqe_data,
                           std::vector<char> &buffer) -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_recv(sqe, fd, buffer.data(), buffer.size(), 0);
    io_uring_sqe_set_data(sqe, sqe_data);
  }

  auto submit_send_request(int fd, sqe_user_data *sqe_data,
                           const std::vector<char> &buffer) -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_send(sqe, fd, buffer.data(), buffer.size(), 0);
    io_uring_sqe_set_data(sqe, sqe_data);
  }

  io_uring ring;
};

class recv_awaitable {
public:
  recv_awaitable(io_uring_handler &io_uring_handler, const int fd,
                 std::vector<char> &buffer)
      : io_uring_handler{io_uring_handler}, fd{fd}, buffer{buffer} {}

  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<>) {
    sqe_user_data.type = sqe_user_data::type::RECV;
    sqe_user_data.fd = fd;
    io_uring_handler.submit_recv_request(fd, &sqe_user_data, buffer);
  }
  size_t await_resume() { return sqe_user_data.result; }

private:
  int fd;
  io_uring_handler &io_uring_handler;
  std::vector<char> &buffer;
  sqe_user_data sqe_user_data;
};

class send_awaitable {
public:
  send_awaitable(io_uring_handler &io_uring_handler, const int fd,
                 const std::vector<char> &buffer)
      : io_uring_handler{io_uring_handler}, fd{fd}, buffer{buffer} {}

  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<>) {
    sqe_user_data.type = sqe_user_data::type::SEND;
    sqe_user_data.fd = fd;
    io_uring_handler.submit_send_request(fd, &sqe_user_data, buffer);
  }
  size_t await_resume() { return sqe_user_data.result; }

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
                   socklen_t *client_address_size)
      : io_uring_handler{io_uring_handler}, fd{fd},
        client_address{client_address},
        client_address_size{client_address_size} {}

  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<>) {
    sqe_user_data.type = sqe_user_data::type::ACCEPT;
    sqe_user_data.fd = fd;
    io_uring_handler.submit_accept_request(
        fd, &sqe_user_data, reinterpret_cast<sockaddr *>(&client_address),
        client_address_size);
  }
  size_t await_resume() { return sqe_user_data.result; }

private:
  int fd;
  io_uring_handler &io_uring_handler;
  sqe_user_data sqe_user_data;
  sockaddr_storage *client_address;
  socklen_t *client_address_size;
};
} // namespace co_uring_http

#endif
