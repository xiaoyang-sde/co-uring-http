#include <coroutine>

#include "http_server.hpp"
#include "socket.hpp"

namespace co_uring_http {
thread_worker::thread_worker(const char *port) : server_socket{} {
  server_socket.bind(port);
  server_socket.listen();
}

auto thread_worker::accept_loop() -> task<> {
  while (true) {
    const int client_fd = co_await server_socket.accept();
    if (client_fd == -1) {
      continue;
    }

    task<> task = handle_client(client_socket(client_fd));
    task.resume();
    task.detach();
  }
}

auto thread_worker::handle_client(client_socket client_socket) -> task<> {
  std::vector<char> read_buffer(1024);
  co_await client_socket.recv(read_buffer);

  std::string header = "HTTP/1.1 200 OK\r\nContent-Length: 4096\r\n\r\n";
  std::string response = header + std::string(4096, ' ');

  std::vector<char> write_buffer(response.cbegin(), response.cend());
  co_await client_socket.send(write_buffer);
}

auto thread_worker::event_loop() -> task<> {
  io_uring_handler &io_uring_handler = io_uring_handler::get_instance();

  task<> task = accept_loop();
  task.resume();
  task.detach();

  while (true) {
    io_uring_handler.submit_and_wait(1);
    io_uring_handler.for_each_cqe([&io_uring_handler](io_uring_cqe *cqe) {
      sqe_user_data *sqe_user_data =
          reinterpret_cast<struct sqe_user_data *>(cqe->user_data);

      switch (sqe_user_data->type) {
      case sqe_user_data::ACCEPT:
      case sqe_user_data::RECV:
      case sqe_user_data::SEND: {
        sqe_user_data->result = cqe->res;
        sqe_user_data->cqe_flags = cqe->flags;
        if (sqe_user_data->coroutine) {
          std::coroutine_handle<>::from_address(sqe_user_data->coroutine)
              .resume();
        }
        break;
      }
      case sqe_user_data::READ:
      case sqe_user_data::WRITE:
        break;
      }

      io_uring_handler.cqe_seen(cqe);
    });
  }
}

http_server::http_server(const size_t thread_count)
    : thread_pool{thread_count} {}

auto http_server::listen(const char *port) -> void {
  const std::function<task<>()> construct_task = [&]() -> task<> {
    co_await thread_pool.schedule();
    co_await thread_worker(port).event_loop();
  };

  std::list<task<>> task_list;
  for (size_t _ = 0; _ < thread_pool.size(); ++_) {
    task_list.emplace_back(construct_task());
    task_list.back().resume();
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}
} // namespace co_uring_http
