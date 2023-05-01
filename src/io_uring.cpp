#include "io_uring.hpp"

#include <liburing.h>
#include <liburing/io_uring.h>
#include <sys/types.h>

#include <stdexcept>

#include "constant.hpp"

namespace co_uring_http {
io_uring::io_uring() {
  if (const int result = io_uring_queue_init(IO_URING_QUEUE_SIZE, &io_uring_, 0); result != 0) {
    throw std::runtime_error("failed to invoke 'io_uring_queue_init'");
  }
}

io_uring::~io_uring() { io_uring_queue_exit(&io_uring_); }

auto io_uring::get_instance() noexcept -> io_uring & {
  thread_local io_uring instance;
  return instance;
}

auto io_uring::for_each_cqe(const std::function<void(io_uring_cqe *)> &lambda) -> void {
  io_uring_cqe *cqe = nullptr;
  unsigned int head = 0;

  io_uring_for_each_cqe(&io_uring_, head, cqe) { lambda(cqe); }
}

auto io_uring::cqe_seen(io_uring_cqe *cqe) -> void { io_uring_cqe_seen(&io_uring_, cqe); }

auto io_uring::submit_and_wait(const int wait_nr) -> int {
  const int result = io_uring_submit_and_wait(&io_uring_, wait_nr);
  if (result < 0) {
    throw std::runtime_error("failed to invoke 'io_uring_submit_and_wait'");
  }
  return result;
}

auto io_uring::submit_multishot_accept_request(
    const int raw_file_descriptor, sqe_data *sqe_data, sockaddr *client_addr, socklen_t *client_len
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_multishot_accept(sqe, raw_file_descriptor, client_addr, client_len, 0);
  io_uring_sqe_set_data(sqe, sqe_data);
}

auto io_uring::submit_recv_request(
    const int raw_file_descriptor, sqe_data *sqe_data, const size_t length
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_recv(sqe, raw_file_descriptor, nullptr, length, 0);
  io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
  io_uring_sqe_set_data(sqe, sqe_data);
  sqe->buf_group = BUFFER_GROUP_ID;
}

auto io_uring::submit_send_request(
    const int raw_file_descriptor, sqe_data *sqe_data, const std::span<char> &buffer,
    const size_t length
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_send(sqe, raw_file_descriptor, buffer.data(), length, 0);
  io_uring_sqe_set_data(sqe, sqe_data);
}

auto io_uring::submit_splice_request(
    sqe_data *sqe_data, int raw_file_descriptor_in, int raw_file_descriptor_out, size_t length
) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_splice(sqe, raw_file_descriptor_in, -1, raw_file_descriptor_out, -1, length, 0);
  io_uring_sqe_set_data(sqe, sqe_data);
}

auto io_uring::submit_cancel_request(sqe_data *sqe_data) -> void {
  io_uring_sqe *sqe = io_uring_get_sqe(&io_uring_);
  io_uring_prep_cancel(sqe, sqe_data, 0);
}

auto io_uring::setup_buffer_ring(
    io_uring_buf_ring *buffer_ring, std::span<std::vector<char>> buffer_list,
    const unsigned int buffer_ring_size
) -> void {
  io_uring_buf_reg io_uring_buf_reg{
      .ring_addr = reinterpret_cast<__uint64_t>(buffer_ring),
      .ring_entries = buffer_ring_size,
      .bgid = BUFFER_GROUP_ID,
  };

  const int result = io_uring_register_buf_ring(&io_uring_, &io_uring_buf_reg, 0);
  if (result != 0) {
    throw std::runtime_error("failed to invoke 'io_uring_register_buf_ring'");
  }
  io_uring_buf_ring_init(buffer_ring);

  const unsigned int mask = io_uring_buf_ring_mask(buffer_ring_size);
  for (unsigned int buffer_id = 0; buffer_id < buffer_ring_size; ++buffer_id) {
    io_uring_buf_ring_add(
        buffer_ring, buffer_list[buffer_id].data(), buffer_list[buffer_id].size(), buffer_id, mask,
        buffer_id
    );
  }
  io_uring_buf_ring_advance(buffer_ring, buffer_ring_size);
};

auto io_uring::add_buffer(
    io_uring_buf_ring *buffer_ring, std::span<char> buffer, const unsigned int buffer_id,
    const unsigned int buffer_ring_size
) -> void {
  const unsigned int mask = io_uring_buf_ring_mask(buffer_ring_size);
  io_uring_buf_ring_add(buffer_ring, buffer.data(), buffer.size(), buffer_id, mask, buffer_id);
  io_uring_buf_ring_advance(buffer_ring, 1);
}
} // namespace co_uring_http
