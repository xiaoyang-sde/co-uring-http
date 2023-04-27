#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "io_uring.hpp"
#include "socket.hpp"
#include "task.hpp"
#include "thread_pool.hpp"

namespace co_uring_http {
class thread_worker {
public:
  thread_worker(const char *port);

  auto accept_loop() -> task<>;

  auto handle_client(socket_file_descriptor client_fd) -> task<>;

  auto event_loop() -> task<>;

private:
  socket_file_descriptor socket;
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
