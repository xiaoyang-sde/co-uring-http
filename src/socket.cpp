#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <unistd.h>

#include "file_descriptor.hpp"
#include "io_uring.hpp"
#include "socket.hpp"

constexpr int LISTEN_QUEUE_SIZE = 512;

namespace co_uring_http {
server_socket::server_socket() {}

auto server_socket::bind(const char *port) -> void {
  addrinfo address_hints;
  addrinfo *socket_address;

  std::memset(&address_hints, 0, sizeof(addrinfo));
  address_hints.ai_family = AF_UNSPEC;
  address_hints.ai_socktype = SOCK_STREAM;
  address_hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, port, &address_hints, &socket_address) != 0) {
    throw std::runtime_error("failed to invoke 'getaddrinfo'");
  }

  for (auto node = socket_address; node != nullptr; node = node->ai_next) {
    fd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
    if (fd.value() == -1) {
      throw std::runtime_error("failed to invoke 'socket'");
    }

    const int flag = 1;
    if (setsockopt(fd.value(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) ==
        -1) {
      throw std::runtime_error("failed to invoke 'setsockopt'");
    }

    if (setsockopt(fd.value(), SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)) ==
        -1) {
      throw std::runtime_error("failed to invoke 'setsockopt'");
    }

    if (::bind(fd.value(), node->ai_addr, node->ai_addrlen) == -1) {
      throw std::runtime_error("failed to invoke 'bind'");
    }
    break;
  }
  freeaddrinfo(socket_address);
}

auto server_socket::listen() -> void {
  if (!fd.has_value()) {
    throw std::runtime_error("the file descriptor is invalid");
  }

  if (::listen(fd.value(), LISTEN_QUEUE_SIZE) == -1) {
    throw std::runtime_error("failed to invoke 'listen'");
  }
}

server_socket::multishot_accept_guard::multishot_accept_guard(
    const int fd, sockaddr_storage *client_address,
    socklen_t *client_address_size
)
    : fd{fd}, client_address{client_address}, client_address_size{
                                                  client_address_size} {}

server_socket::multishot_accept_guard::~multishot_accept_guard() {
  io_uring_handler::get_instance().submit_cancel_request(&sqe_user_data);
}

auto server_socket::multishot_accept_guard::await_ready() -> bool {
  return false;
}

auto server_socket::multishot_accept_guard::await_suspend(
    std::coroutine_handle<> coroutine
) -> void {
  if (initial_await) {
    sqe_user_data.type = sqe_user_data::type::ACCEPT;
    sqe_user_data.coroutine = coroutine.address();

    io_uring_handler::get_instance().submit_multishot_accept_request(
        fd, &sqe_user_data, reinterpret_cast<sockaddr *>(client_address),
        client_address_size
    );
    initial_await = false;
  }
}

auto server_socket::multishot_accept_guard::await_resume() -> int {
  if (!(sqe_user_data.cqe_flags & IORING_CQE_F_MORE)) {
    io_uring_handler::get_instance().submit_multishot_accept_request(
        fd, &sqe_user_data, reinterpret_cast<sockaddr *>(client_address),
        client_address_size
    );
  }
  return sqe_user_data.result;
}

auto server_socket::accept(
    sockaddr_storage *client_address, socklen_t *client_address_size
) -> class multishot_accept_guard & {
  if (!fd.has_value()) {
    throw std::runtime_error("the file descriptor is invalid");
  }

  if (!multishot_accept_guard.has_value()) {
    multishot_accept_guard.emplace(
        fd.value(), client_address, client_address_size
    );
  }
  return multishot_accept_guard.value();
}

client_socket::client_socket(const int fd) : file_descriptor{fd} {}

client_socket::recv_awaitable::recv_awaitable(
    const int fd, std::vector<char> &buffer
)
    : fd{fd}, buffer{buffer} {}

auto client_socket::recv_awaitable::await_ready() -> bool { return false; }

auto client_socket::recv_awaitable::await_suspend(
    std::coroutine_handle<> coroutine
) -> void {
  sqe_user_data.type = sqe_user_data::type::RECV;
  sqe_user_data.coroutine = coroutine.address();

  io_uring_handler::get_instance().submit_recv_request(
      fd, &sqe_user_data, buffer
  );
}

auto client_socket::recv_awaitable::await_resume() -> size_t {
  return sqe_user_data.result;
}

auto client_socket::recv(std::vector<char> &buffer) -> recv_awaitable {
  if (fd.has_value()) {
    return recv_awaitable(fd.value(), buffer);
  }
  throw std::runtime_error("the file descriptor is invalid");
}

client_socket::send_awaitable::send_awaitable(
    const int fd, const std::vector<char> &buffer
)
    : fd{fd}, buffer{buffer} {};

auto client_socket::send_awaitable::await_ready() -> bool { return false; }

auto client_socket::send_awaitable::await_suspend(
    std::coroutine_handle<> coroutine
) -> void {
  sqe_user_data.type = sqe_user_data::type::SEND;
  sqe_user_data.coroutine = coroutine.address();

  io_uring_handler::get_instance().submit_send_request(
      fd, &sqe_user_data, buffer
  );
}

auto client_socket::send_awaitable::await_resume() -> size_t {
  return sqe_user_data.result;
}

auto client_socket::send(const std::vector<char> &buffer) -> send_awaitable {
  if (fd.has_value()) {
    return send_awaitable(fd.value(), buffer);
  }
  throw std::runtime_error("the file descriptor is invalid");
}

} // namespace co_uring_http
