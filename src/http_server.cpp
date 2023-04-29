#include "http_server.hpp"
#include "buffer_ring.hpp"
#include "socket.hpp"

namespace co_uring_http {
thread_worker::thread_worker(const char *port) {
  buffer_ring::get_instance().register_buffer_ring(BUFFER_RING_SIZE, BUFFER_SIZE);

  server_socket_.bind(port);
  server_socket_.listen();
}

auto thread_worker::accept_loop() -> task<> {
  while (true) {
    const int raw_file_descriptor = co_await server_socket_.accept();
    if (raw_file_descriptor == -1) {
      continue;
    }

    task<> task = handle_client(client_socket(raw_file_descriptor));
    task.resume();
    task.detach();
  }
}

auto thread_worker::handle_client(client_socket client_socket) -> task<> {
  buffer_ring &buffer_ring = buffer_ring::get_instance();
  while (true) {
    const auto [buffer_id, buffer_size] = co_await client_socket.recv(BUFFER_SIZE);
    if (buffer_size == 0) {
      break;
    }

    const std::span<std::byte> buffer = buffer_ring.borrow_buffer(buffer_id, buffer_size);
    co_await client_socket.send(buffer, buffer_size);
    buffer_ring.return_buffer(buffer_id);
  }
}

auto thread_worker::event_loop() -> task<> {
  io_uring_handler &io_uring_handler = io_uring_handler::get_instance();

  task<> task = accept_loop();
  task.resume();
  task.detach();

  while (true) {
    io_uring_handler.submit_and_wait(1);
    io_uring_handler.for_each_cqe([&io_uring_handler](io_uring_cqe *cqe) {
      auto *sqe_data = reinterpret_cast<struct sqe_data *>(io_uring_cqe_get_data(cqe));

      sqe_data->cqe_res = cqe->res;
      sqe_data->cqe_flags = cqe->flags;
      if (sqe_data->coroutine != nullptr) {
        std::coroutine_handle<>::from_address(sqe_data->coroutine).resume();
      }

      io_uring_handler.cqe_seen(cqe);
    });
  }
}

http_server::http_server(const size_t thread_count) : thread_pool_{thread_count} {}

auto http_server::listen(const char *port) -> void {
  const std::function<task<>()> construct_task = [&]() -> task<> {
    co_await thread_pool_.schedule();
    co_await thread_worker(port).event_loop();
  };

  std::list<task<>> task_list;
  for (size_t _ = 0; _ < thread_pool_.size(); ++_) {
    task_list.emplace_back(construct_task());
    task_list.back().resume();
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}
} // namespace co_uring_http
