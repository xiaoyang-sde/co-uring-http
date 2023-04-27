#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <coroutine>
#include <vector>

#include "file_descriptor.hpp"
#include "io_uring.hpp"

namespace co_uring_http {

class socket_file_descriptor : public file_descriptor {
public:
  socket_file_descriptor();

  explicit socket_file_descriptor(const int fd);

  auto bind(const char *port) -> void;

  auto listen() -> void;

  class recv_awaitable {
  public:
    recv_awaitable(const int fd, std::vector<char> &buffer);

    auto await_ready() -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() -> size_t;

  private:
    int fd;
    std::vector<char> &buffer;
    sqe_user_data sqe_user_data;
  };

  auto recv(std::vector<char> &buffer) -> recv_awaitable;

  class send_awaitable {
  public:
    send_awaitable(const int fd, const std::vector<char> &buffer);

    auto await_ready() -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() -> size_t;

  private:
    const int fd;
    const std::vector<char> &buffer;
    sqe_user_data sqe_user_data;
  };

  auto send(const std::vector<char> &buffer) -> send_awaitable;

  class accept_awaitable {
  public:
    accept_awaitable(
        const int fd, sockaddr_storage *client_address,
        socklen_t *client_address_size
    );

    auto await_ready() -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() -> int;

  private:
    const int fd;
    sqe_user_data sqe_user_data;
    sockaddr_storage *client_address;
    socklen_t *client_address_size;
  };

  auto accept(sockaddr_storage *client_address, socklen_t *client_address_size)
      -> accept_awaitable;
};
} // namespace co_uring_http

#endif
