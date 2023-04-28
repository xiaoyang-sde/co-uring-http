#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <coroutine>
#include <vector>

#include "file_descriptor.hpp"
#include "io_uring.hpp"

namespace co_uring_http {

class server_socket : public file_descriptor {
public:
  server_socket();

  auto bind(const char *port) -> void;

  auto listen() -> void;

  class multishot_accept_guard {
  public:
    multishot_accept_guard(
        const int fd, sockaddr_storage *client_address,
        socklen_t *client_address_size
    );
    ~multishot_accept_guard();

    auto await_ready() -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() -> int;

  private:
    bool initial_await = true;
    const int fd;
    sockaddr_storage *client_address;
    socklen_t *client_address_size;
    sqe_user_data sqe_user_data;
  };

  auto accept(
      sockaddr_storage *client_address = nullptr,
      socklen_t *client_address_size = nullptr
  ) -> multishot_accept_guard &;

private:
  std::optional<multishot_accept_guard> multishot_accept_guard;
};

class client_socket : public file_descriptor {
public:
  explicit client_socket(const int fd);

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
};

} // namespace co_uring_http

#endif
