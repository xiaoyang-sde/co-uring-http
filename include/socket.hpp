#ifndef SOCKET_HPP
#define SOCKET_HPP
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

constexpr int LISTEN_QUEUE_SIZE = 512;

namespace co_uring_http {
class socket_handler {
public:
  socket_handler(const char *port) {
    addrinfo address_hints;
    addrinfo *socket_address;

    memset(&address_hints, 0, sizeof(addrinfo));
    address_hints.ai_family = AF_UNSPEC;
    address_hints.ai_socktype = SOCK_STREAM;
    address_hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &address_hints, &socket_address) != 0) {
      throw std::runtime_error("failed to invoke 'getaddrinfo'");
    }

    for (auto node = socket_address; node != nullptr; node = node->ai_next) {
      socket_fd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
      if (socket_fd == -1) {
        throw std::runtime_error("failed to invoke 'socket'");
      }

      const int flag = 1;
      if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag,
                     sizeof(flag)) == -1) {
        throw std::runtime_error("failed to invoke 'setsockopt'");
      }

      if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &flag,
                     sizeof(flag)) == -1) {
        throw std::runtime_error("failed to invoke 'setsockopt'");
      }

      if (::bind(socket_fd, node->ai_addr, node->ai_addrlen) == -1) {
        throw std::runtime_error("failed to invoke 'bind'");
      }
      break;
    }
    freeaddrinfo(socket_address);

    if (::listen(socket_fd, LISTEN_QUEUE_SIZE) == -1) {
      throw std::runtime_error("failed to invoke 'listen'");
    }
  }

  ~socket_handler() { close(socket_fd); }

  auto get_socket_fd() const noexcept -> int { return socket_fd; }

private:
  int socket_fd;
};
} // namespace co_uring_http

#endif
