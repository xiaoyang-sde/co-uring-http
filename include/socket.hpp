#ifndef SOCKET_HPP
#define SOCKET_HPP

namespace co_uring_http {
class socket_handler {
public:
  socket_handler(const char *port);
  ~socket_handler();

  auto get_socket_fd() const noexcept -> int;

private:
  int socket_fd;
};
} // namespace co_uring_http

#endif
