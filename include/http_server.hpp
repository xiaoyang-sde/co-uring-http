#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "io_uring.hpp"
#include "socket.hpp"
#include "sync_wait.hpp"
#include "task.hpp"
#include "thread_pool.hpp"
#include <coroutine>
#include <functional>
#include <iostream>
#include <map>
#include <thread>

namespace co_uring_http {
class thread_worker {
public:
  thread_worker(const char *port)
      : io_uring_handler{2048}, socket_handler{port} {}

  auto get_socket_fd() const noexcept -> int {
    return socket_handler.get_socket_fd();
  }

  auto accept_loop() -> task<> {
    while (true) {
      sockaddr_storage client_address;
      socklen_t client_address_size = sizeof(client_address);

      const int client_fd =
          co_await accept_awaitable(io_uring_handler, get_socket_fd(),
                                    &client_address, &client_address_size);
      client_map.insert_or_assign(client_fd, handle_client(client_fd));
      client_map[client_fd].resume();
    }
  }

  auto handle_client(const int client_fd) -> task<> {
    std::vector<char> read_buffer(1024);
    co_await recv_awaitable(io_uring_handler, client_fd, read_buffer);

    std::string header = "HTTP/1.1 200 OK\r\nContent-Length: 4096\r\n\r\n";
    std::string response = header + std::string(4096, ' ');

    std::vector<char> write_buffer(response.cbegin(), response.cend());
    co_await send_awaitable(io_uring_handler, client_fd, write_buffer);
    close(client_fd);
  }

  auto event_loop() -> task<> {
    client_map.insert_or_assign(get_socket_fd(), accept_loop());
    client_map[get_socket_fd()].resume();

    while (true) {
      io_uring_handler.submit_and_wait(1);
      io_uring_handler.for_each_cqe([this](io_uring_cqe *cqe) {
        sqe_user_data *sqe_user_data =
            reinterpret_cast<struct sqe_user_data *>(cqe->user_data);

        switch (sqe_user_data->type) {
        case sqe_user_data::ACCEPT:
        case sqe_user_data::RECV:
        case sqe_user_data::SEND: {
          sqe_user_data->result = cqe->res;
          if (client_map.contains(sqe_user_data->fd)) {
            client_map[sqe_user_data->fd].resume();
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

private:
  std::map<int, task<>> client_map;
  io_uring_handler io_uring_handler;
  socket_handler socket_handler;
};

class http_server {
public:
  http_server(const size_t thread_count = std::thread::hardware_concurrency())
      : thread_pool{thread_count} {}

  auto listen(const char *port) -> void {
    const std::function<task<>()> construct_task = [&]() -> task<> {
      co_await thread_pool.schedule();
      co_await thread_worker(port).event_loop();
    };

    std::list<task<>> task_list;
    for (int _ = 0; _ < thread_pool.size(); ++_) {
      task_list.emplace_back(construct_task());
      task_list.back().resume();
    }

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }
  }

private:
  thread_pool thread_pool;
};
} // namespace co_uring_http

#endif
