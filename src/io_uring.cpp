#include "io_uring.hpp"
#include <liburing.h>
#include <stdexcept>

constexpr size_t IO_URING_QUEUE_SIZE = 2048;

namespace co_uring_http {

io_uring_handler::io_uring_handler() {
  if (const int result =
          io_uring_queue_init(IO_URING_QUEUE_SIZE, &io_uring_, 0);
      result != 0) {
    throw std::runtime_error("failed to invoke 'io_uring_queue_init'");
  }
}

io_uring_handler::~io_uring_handler() { io_uring_queue_exit(&io_uring_); }

auto io_uring_handler::get_instance() noexcept -> io_uring_handler & {
  thread_local io_uring_handler instance;
  return instance;
}

auto io_uring_handler::get_uring() noexcept -> io_uring & { return io_uring_; }

auto io_uring_handler::for_each_cqe(std::function<void(io_uring_cqe *)> functor)
    -> void {
  io_uring_cqe *cqe;
  unsigned int head;

  io_uring_for_each_cqe(&io_uring_, head, cqe) { functor(cqe); }
}

auto io_uring_handler::cqe_seen(io_uring_cqe *cqe) -> void {
  io_uring_cqe_seen(&io_uring_, cqe);
}

auto io_uring_handler::submit_and_wait(const int wait_nr) -> int {
  const int result = io_uring_submit_and_wait(&io_uring_, wait_nr);
  if (result < 0) {
    throw std::runtime_error("failed to invoke 'io_uring_submit_and_wait'");
  }
  return result;
}

auto io_uring_handler::submit_multishot_accept_request(
    int fd, sqe_data *sqe_data, sockaddr *client_addr, socklen_t *client_len
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_multishot_accept(sqe, fd, client_addr, client_len, 0);
  io_uring_sqe_set_data(sqe, sqe_data);
}

auto io_uring_handler::submit_recv_request(
    int fd, sqe_data *sqe_data, std::vector<char> &buffer
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_recv(sqe, fd, buffer.data(), buffer.size(), 0);
  io_uring_sqe_set_data(sqe, sqe_data);
}

auto io_uring_handler::submit_send_request(
    int fd, sqe_data *sqe_data, const std::vector<char> &buffer
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_send(sqe, fd, buffer.data(), buffer.size(), 0);
  io_uring_sqe_set_data(sqe, sqe_data);
}

auto io_uring_handler::submit_cancel_request(sqe_data *sqe_data) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_cancel(sqe, sqe_data, 0);
}

} // namespace co_uring_http
