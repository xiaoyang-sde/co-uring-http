module;

#include <liburing.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

export module co_uring_http.io:socket;

import std;

import co_uring_http.coroutine;

import :constant;
import :file;
import :io_uring;
import :error;

namespace co_uring_http {

export class socket_client : public file {
public:
  using file::file;

  class recv_awaiter {
  public:
    recv_awaiter(const std::uint_least32_t raw_fd) noexcept : raw_fd_{raw_fd} {}

    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto
    await_suspend(const std::coroutine_handle<> coroutine) noexcept -> void {
      sqe_data_.coroutine = coroutine.address();
      io_uring_context::get_instance().submit_recv_request(&sqe_data_, raw_fd_);
    }

    [[nodiscard]] auto await_resume() const noexcept
        -> std::expected<std::tuple<std::uint_least32_t, std::uint_least32_t>,
                         std::uint_least16_t> {
      const std::uint_least32_t buf_id =
          sqe_data_.cqe_flags >> IORING_CQE_BUFFER_SHIFT;
      return decode(sqe_data_.cqe_res)
          .transform([buf_id](const std::uint_least32_t buf_size) noexcept {
            return std::tuple<std::uint_least32_t, std::uint_least32_t>{
                buf_id, buf_size};
          });
    }

  private:
    sqe_data sqe_data_;
    const std::uint_least32_t raw_fd_;
  };

  [[nodiscard]] auto recv() const noexcept -> recv_awaiter {
    return recv_awaiter{*raw_fd_};
  }

  class send_awaiter {
  public:
    send_awaiter(const std::uint_least32_t raw_fd,
                 const std::span<const std::uint_least8_t> buf) noexcept
        : raw_fd_{raw_fd}, buf_{buf} {}

    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto
    await_suspend(const std::coroutine_handle<> coroutine) noexcept -> void {
      sqe_data_.coroutine = coroutine.address();
      io_uring_context::get_instance().submit_send_request(&sqe_data_, raw_fd_,
                                                           buf_);
    }

    [[nodiscard]] auto await_resume() const noexcept
        -> std::expected<std::uint_least32_t, std::uint_least16_t> {
      return decode(sqe_data_.cqe_res);
    }

  private:
    sqe_data sqe_data_;
    const std::uint_least32_t raw_fd_;
    const std::span<const std::uint_least8_t> buf_;
  };

  [[nodiscard]] auto
  send(const std::span<const std::uint_least8_t> buf) const noexcept
      -> task<std::expected<std::uint_least32_t, std::uint_least16_t>> {
    std::uint_least32_t state = 0;
    while (state < buf.size()) {
      auto send_result = co_await send_awaiter{*raw_fd_, buf.subspan(state)};
      if (!send_result.has_value()) {
        co_return std::unexpected{send_result.error()};
      }
      state += *send_result;
    }
    co_return state;
  }
};

export class socket_server : public file {
public:
  using file::file;

  auto listen() const noexcept -> std::expected<void, std::uint_least16_t> {
    return decode_void(::listen(*raw_fd_, SOCKET_LISTEN_QUEUE_SIZE));
  }

  class multishot_accept_guard {
  public:
    multishot_accept_guard(const std::uint_least32_t raw_fd,
                           sockaddr_storage *const client_address,
                           socklen_t *const client_address_size) noexcept
        : initial_await_{true}, raw_fd_{raw_fd},
          client_address_{client_address},
          client_address_size_{client_address_size} {}

    ~multishot_accept_guard() noexcept {
      io_uring_context::get_instance().submit_cancel_request(&sqe_data_);
    }

    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto
    await_suspend(const std::coroutine_handle<> coroutine) noexcept -> void {
      sqe_data_.coroutine = coroutine.address();
      if (initial_await_) {
        io_uring_context::get_instance().submit_multishot_accept_request(
            &sqe_data_, raw_fd_, reinterpret_cast<sockaddr *>(client_address_),
            client_address_size_);
        initial_await_ = false;
      }
    }

    [[nodiscard]] auto await_resume() const noexcept
        -> std::expected<socket_client, std::uint_least16_t> {
      return decode(sqe_data_.cqe_res)
          .transform([](const std::uint_least32_t raw_fd) -> socket_client {
            return socket_client{raw_fd};
          });
    }

  private:
    sqe_data sqe_data_;
    bool initial_await_;
    const std::uint_least32_t raw_fd_;
    sockaddr_storage *const client_address_;
    socklen_t *const client_address_size_;
  };

  [[nodiscard]] auto accept(sockaddr_storage *const client_address = nullptr,
                            socklen_t *const client_address_size =
                                nullptr) noexcept -> multishot_accept_guard & {
    if (!multishot_accept_guard_.has_value()) {
      multishot_accept_guard_.emplace(*raw_fd_, client_address,
                                      client_address_size);
    }
    return *multishot_accept_guard_;
  }

private:
  std::optional<multishot_accept_guard> multishot_accept_guard_;
};

export [[nodiscard]] auto
bind() noexcept -> std::expected<socket_server, std::uint_least16_t> {
  addrinfo address_hints;
  addrinfo *socket_address;
  std::memset(&address_hints, 0, sizeof(addrinfo));
  address_hints.ai_family = AF_UNSPEC;
  address_hints.ai_socktype = SOCK_STREAM;
  address_hints.ai_flags = AI_PASSIVE;

  const std::string port = std::format("{}", PORT);
  const auto getaddrinfo_result = decode_void(
      getaddrinfo(nullptr, port.data(), &address_hints, &socket_address));
  if (!getaddrinfo_result.has_value()) {
    return std::unexpected{getaddrinfo_result.error()};
  }

  for (auto node = socket_address; node != nullptr; node = node->ai_next) {
    const auto socket_result =
        decode(socket(node->ai_family, node->ai_socktype, node->ai_protocol));
    if (!socket_result.has_value()) {
      return std::unexpected{socket_result.error()};
    }

    const std::uint_least32_t raw_fd = *socket_result;
    const std::uint_least32_t flag = 1;
    {
      const auto setsockopt_result = decode_void(
          setsockopt(raw_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)));
      if (!setsockopt_result.has_value()) {
        return std::unexpected{setsockopt_result.error()};
      }
    }

    {
      const auto setsockopt_result = decode_void(
          setsockopt(raw_fd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)));
      if (!setsockopt_result.has_value()) {
        return std::unexpected{setsockopt_result.error()};
      }
    }

    const auto bind_result =
        decode_void(::bind(raw_fd, node->ai_addr, node->ai_addrlen));
    if (!bind_result.has_value()) {
      return std::unexpected{bind_result.error()};
    }

    freeaddrinfo(socket_address);
    return socket_server{raw_fd};
  }
}

} // namespace co_uring_http
