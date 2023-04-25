#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "io_uring.hpp"
#include "socket.hpp"
#include "task.hpp"
#include "thread_pool.hpp"

#include <map>

namespace co_uring_http {
class thread_worker {
public:
  thread_worker(const char *port);

  auto get_socket_fd() const noexcept -> int;

  auto accept_loop() -> task<>;

  auto handle_client(const int client_fd) -> task<>;

  auto event_loop() -> task<>;

private:
  std::map<int, task<>> client_map;
  io_uring_handler io_uring_handler;
  socket_handler socket_handler;
};

class http_server {
public:
  http_server(const size_t thread_count = std::thread::hardware_concurrency());

  auto listen(const char *port) -> void;

private:
  thread_pool thread_pool;
};
} // namespace co_uring_http

#endif
