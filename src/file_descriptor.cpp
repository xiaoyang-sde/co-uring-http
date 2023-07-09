#include "file_descriptor.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <optional>
#include <stdexcept>
#include <utility>

namespace co_uring_http {
file_descriptor::file_descriptor() = default;

file_descriptor::file_descriptor(const int raw_file_descriptor)
    : raw_file_descriptor_{raw_file_descriptor} {}

file_descriptor::~file_descriptor() {
  if (raw_file_descriptor_.has_value()) {
    close(raw_file_descriptor_.value());
  }
}

file_descriptor::file_descriptor(file_descriptor &&other) noexcept
    : raw_file_descriptor_{other.raw_file_descriptor_} {
  other.raw_file_descriptor_ = std::nullopt;
}

auto file_descriptor::operator=(file_descriptor &&other) noexcept -> file_descriptor & {
  if (this == std::addressof(other)) {
    return *this;
  }
  raw_file_descriptor_ = std::exchange(other.raw_file_descriptor_, std::nullopt);
  return *this;
}

auto file_descriptor::operator<=>(const file_descriptor &other) const -> std::strong_ordering {
  return raw_file_descriptor_.value() <=> other.raw_file_descriptor_.value();
}

auto file_descriptor::get_raw_file_descriptor() const -> int {
  return raw_file_descriptor_.value();
}

splice_awaiter::splice_awaiter(
    const int raw_file_descriptor_in, const int raw_file_descriptor_out, const size_t length
)
    : raw_file_descriptor_in_{raw_file_descriptor_in},
      raw_file_descriptor_out_{raw_file_descriptor_out}, length_{length} {}

auto splice_awaiter::await_ready() const -> bool { return false; }

auto splice_awaiter::await_suspend(std::coroutine_handle<> coroutine) -> void {
  sqe_data_.coroutine = coroutine.address();

  io_uring::get_instance().submit_splice_request(
      &sqe_data_, raw_file_descriptor_in_, raw_file_descriptor_out_, length_
  );
}

auto splice_awaiter::await_resume() const -> ssize_t { return sqe_data_.cqe_res; }

auto pipe() -> std::tuple<file_descriptor, file_descriptor> {
  std::array<int, 2> fd;
  if (::pipe(fd.data()) == -1) {
    throw std::runtime_error("failed to invoke 'pipe'");
  }
  return {file_descriptor{fd[0]}, file_descriptor{fd[1]}};
};

auto splice(
    const file_descriptor &file_descriptor_in, const file_descriptor &file_descriptor_out,
    const size_t length
) -> task<ssize_t> {
  const auto [read_pipe, write_pipe] = pipe();

  size_t bytes_sent = 0;
  while (bytes_sent < length) {
    {
      ssize_t result = co_await splice_awaiter(
          file_descriptor_in.get_raw_file_descriptor(), write_pipe.get_raw_file_descriptor(), length
      );
      if (result < 0) {
        co_return -1;
      }
    }
    {
      ssize_t result = co_await splice_awaiter(
          read_pipe.get_raw_file_descriptor(), file_descriptor_out.get_raw_file_descriptor(), length
      );
      if (result < 0) {
        co_return -1;
      }
      bytes_sent += result;
    }
  }
  co_return bytes_sent;
}

auto open(const std::filesystem::path &path) -> file_descriptor {
  const int raw_file_descriptor = ::open(path.c_str(), O_RDONLY);
  if (raw_file_descriptor == -1) {
    throw std::runtime_error("failed to invoke 'open'");
  }
  return file_descriptor{raw_file_descriptor};
}
} // namespace co_uring_http
