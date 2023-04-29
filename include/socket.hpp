#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <coroutine>
#include <span>
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
    bool initial_await_ = true;
    const int fd_;
    sockaddr_storage *client_address_;
    socklen_t *client_address_size_;
    sqe_data sqe_data_;
  };

  auto accept(
      sockaddr_storage *client_address = nullptr,
      socklen_t *client_address_size = nullptr
  ) -> multishot_accept_guard &;

private:
  std::optional<multishot_accept_guard> multishot_accept_guard_;
};

class client_socket : public file_descriptor {
public:
  explicit client_socket(const int fd);

  class recv_awaiter {
  public:
    recv_awaiter(const int fd, const size_t length);

    auto await_ready() -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() -> std::tuple<unsigned int, size_t>;

  private:
    const int fd_;
    const size_t length_;
    sqe_data sqe_data_;
  };

  auto recv(const size_t length) -> recv_awaiter;

  class send_awaiter {
  public:
    send_awaiter(
        const int fd, const std::span<std::byte> &buffer, const size_t length
    );

    auto await_ready() -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() -> size_t;

  private:
    const int fd_;
    const size_t length_;
    const std::span<std::byte> &buffer_;
    sqe_data sqe_data_;
  };

  auto send(const std::span<std::byte> &buffer, const size_t length)
      -> send_awaiter;
};

} // namespace co_uring_http

#endif
