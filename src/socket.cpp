#include "socket.hpp"

#include <netdb.h>
#include <unistd.h>

#include <cstring>
#include <span>
#include <stdexcept>

#include "buffer_ring.hpp"
#include "file_descriptor.hpp"

namespace co_uring_http {
server_socket::server_socket() = default;

auto server_socket::bind(const char *port) -> void {
  addrinfo address_hints;
  addrinfo *socket_address;

  std::memset(&address_hints, 0, sizeof(addrinfo));
  address_hints.ai_family = AF_UNSPEC;
  address_hints.ai_socktype = SOCK_STREAM;
  address_hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(nullptr, port, &address_hints, &socket_address) != 0) {
    throw std::runtime_error("failed to invoke 'getaddrinfo'");
  }

  for (auto *node = socket_address; node != nullptr; node = node->ai_next) {
    raw_file_descriptor_ = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
    if (raw_file_descriptor_.value() == -1) {
      throw std::runtime_error("failed to invoke 'socket'");
    }

    const int flag = 1;
    if (setsockopt(raw_file_descriptor_.value(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) ==
        -1) {
      throw std::runtime_error("failed to invoke 'setsockopt'");
    }

    if (setsockopt(raw_file_descriptor_.value(), SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)) ==
        -1) {
      throw std::runtime_error("failed to invoke 'setsockopt'");
    }

    if (::bind(raw_file_descriptor_.value(), node->ai_addr, node->ai_addrlen) == -1) {
      throw std::runtime_error("failed to invoke 'bind'");
    }
    break;
  }
  freeaddrinfo(socket_address);
}

auto server_socket::listen() const -> void {
  if (!raw_file_descriptor_.has_value()) {
    throw std::runtime_error("the file descriptor is invalid");
  }

  if (::listen(raw_file_descriptor_.value(), SOCKET_LISTEN_QUEUE_SIZE) == -1) {
    throw std::runtime_error("failed to invoke 'listen'");
  }
}

server_socket::multishot_accept_guard::multishot_accept_guard(
    const int raw_file_descriptor, sockaddr_storage *client_address, socklen_t *client_address_size
)
    : raw_file_descriptor_{raw_file_descriptor}, client_address_{client_address},
      client_address_size_{client_address_size} {}

server_socket::multishot_accept_guard::~multishot_accept_guard() {
  io_uring_handler::get_instance().submit_cancel_request(&sqe_data_);
}

auto server_socket::multishot_accept_guard::await_ready() const -> bool { return false; }

auto server_socket::multishot_accept_guard::await_suspend(std::coroutine_handle<> coroutine)
    -> void {
  if (initial_await_) {
    sqe_data_.coroutine = coroutine.address();

    io_uring_handler::get_instance().submit_multishot_accept_request(
        raw_file_descriptor_, &sqe_data_, reinterpret_cast<sockaddr *>(client_address_),
        client_address_size_
    );
    initial_await_ = false;
  }
}

auto server_socket::multishot_accept_guard::await_resume() -> int {
  if (!(sqe_data_.cqe_flags & IORING_CQE_F_MORE)) {
    io_uring_handler::get_instance().submit_multishot_accept_request(
        raw_file_descriptor_, &sqe_data_, reinterpret_cast<sockaddr *>(client_address_),
        client_address_size_
    );
  }
  return sqe_data_.cqe_res;
}

auto server_socket::accept(sockaddr_storage *client_address, socklen_t *client_address_size)
    -> multishot_accept_guard & {
  if (!raw_file_descriptor_.has_value()) {
    throw std::runtime_error("the file descriptor is invalid");
  }

  if (!multishot_accept_guard_.has_value()) {
    multishot_accept_guard_.emplace(
        raw_file_descriptor_.value(), client_address, client_address_size
    );
  }
  return multishot_accept_guard_.value();
}

client_socket::client_socket(const int raw_file_descriptor)
    : file_descriptor{raw_file_descriptor} {}

client_socket::recv_awaiter::recv_awaiter(const int raw_file_descriptor, const size_t length)
    : raw_file_descriptor_{raw_file_descriptor}, length_{length} {}

auto client_socket::recv_awaiter::await_ready() const -> bool { return false; }

auto client_socket::recv_awaiter::await_suspend(std::coroutine_handle<> coroutine) -> void {
  sqe_data_.coroutine = coroutine.address();
  io_uring_handler::get_instance().submit_recv_request(raw_file_descriptor_, &sqe_data_, length_);
}

auto client_socket::recv_awaiter::await_resume() -> std::tuple<unsigned int, size_t> {
  if (sqe_data_.cqe_flags | IORING_CQE_F_BUFFER) {
    const unsigned int buffer_id = sqe_data_.cqe_flags >> IORING_CQE_BUFFER_SHIFT;
    return {buffer_id, sqe_data_.cqe_res};
  }
  return {};
}

auto client_socket::recv(const size_t length) -> recv_awaiter {
  if (raw_file_descriptor_.has_value()) {
    return {raw_file_descriptor_.value(), length};
  }
  throw std::runtime_error("the file descriptor is invalid");
}

client_socket::send_awaiter::send_awaiter(
    const int raw_file_descriptor, const std::span<std::byte> &buffer, const size_t length
)
    : raw_file_descriptor_{raw_file_descriptor}, length_{length}, buffer_{buffer} {};

auto client_socket::send_awaiter::await_ready() const -> bool { return false; }

auto client_socket::send_awaiter::await_suspend(std::coroutine_handle<> coroutine) -> void {
  sqe_data_.coroutine = coroutine.address();

  io_uring_handler::get_instance().submit_send_request(
      raw_file_descriptor_, &sqe_data_, buffer_, length_
  );
}

auto client_socket::send_awaiter::await_resume() const -> size_t { return sqe_data_.cqe_res; }

auto client_socket::send(const std::span<std::byte> &buffer, const size_t length) -> send_awaiter {
  if (raw_file_descriptor_.has_value()) {
    return {raw_file_descriptor_.value(), buffer, length};
  }
  throw std::runtime_error("the file descriptor is invalid");
}

} // namespace co_uring_http
