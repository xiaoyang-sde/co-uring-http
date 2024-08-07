export module co_uring_http.http:server;

import std;

import co_uring_http.common;
import co_uring_http.coroutine;
import co_uring_http.io;

import :message;

namespace co_uring_http {

class worker {
public:
  auto init() noexcept -> void {
    unwrap(io_uring_context::get_instance().queue_init());
    unwrap(buf_ring::get_instance().register_buf_ring());

    socket_server_.emplace(unwrap(bind()));
    unwrap(socket_server_->listen());
    spawn(accept_client());
  }

  [[nodiscard]] auto accept_client() noexcept -> task<> {
    while (true) {
      spawn(handle_client(unwrap(co_await socket_server_->accept())));
    }
  }

  [[nodiscard]] auto
  handle_client(socket_client socket_client) const noexcept -> task<> {
    http_parser http_parser;

    while (true) {
      const auto [recv_buf_id, recv_buf_size] =
          unwrap(co_await socket_client.recv());
      if (recv_buf_size == 0) {
        break;
      }

      const std::span<std::uint_least8_t> recv_buf =
          buf_ring::get_instance()
              .borrow_buf(recv_buf_id)
              .subspan(0, recv_buf_size);
      if (const auto parse_result = http_parser.consume_packet(recv_buf);
          parse_result.has_value()) {
        const http_request &http_request = *parse_result;
        const std::filesystem::path file_path =
            std::filesystem::relative(http_request.url, "/");

        http_response http_response;
        http_response.version = http_request.version;
        if (std::filesystem::is_regular_file(file_path)) {
          http_response.status = "200";
          http_response.status_text = "OK";
          const std::uint_least32_t file_size =
              std::filesystem::file_size(file_path);
          http_response.headers.emplace_back("content-length",
                                             std::format("{}", file_size));

          const std::string send_buf = std::format("{}", http_response);
          unwrap(co_await socket_client.send(
              {reinterpret_cast<const std::uint_least8_t *>(send_buf.data()),
               send_buf.size()}));

          const file f = unwrap(open(file_path.c_str()));
          unwrap(co_await splice(f, socket_client, file_size));
        } else {
          http_response.status = "404";
          http_response.status_text = "Not Found";
          http_response.headers.emplace_back("content-length", "0");

          const std::string send_buf = std::format("{}", http_response);
          unwrap(co_await socket_client.send(
              {reinterpret_cast<const std::uint_least8_t *>(send_buf.data()),
               send_buf.size()}));
        }
      }

      buf_ring::get_instance().return_buf(recv_buf_id);
    }
  }

private:
  deferred_initialization<socket_server> socket_server_;
};

export class http_server {
public:
  auto listen() -> void {
    auto worker_list = std::views::iota(0) |
                       std::views::take(thread_pool_.size()) |
                       std::views::transform([this](auto) noexcept -> task<> {
                         return [this]() noexcept -> task<> {
                           co_await thread_pool_.schedule();
                           worker worker;
                           worker.init();
                           io_uring_context::get_instance().event_loop();
                         }();
                       }) |
                       std::ranges::to<std::vector<task<>>>();
    std::ranges::for_each(worker_list, spawn<task<> &>);
    std::ranges::for_each(worker_list, wait<task<> &>);
  }

private:
  thread_pool thread_pool_;
};

} // namespace co_uring_http
