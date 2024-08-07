module;

#include <liburing.h>
#include <unistd.h>

export module co_uring_http.io:file;

import std;

import co_uring_http.coroutine;

import :io_uring;
import :error;

namespace co_uring_http {

export class file {
public:
  explicit file(const std::uint_least32_t raw_fd) noexcept : raw_fd_{raw_fd} {}

  file(file &&other) noexcept
      : raw_fd_{std::exchange(other.raw_fd_, std::nullopt)} {}

  ~file() noexcept {
    if (raw_fd_.has_value()) {
      ::close(*raw_fd_);
    }
  }

  [[nodiscard]] auto get_raw_fd() const noexcept -> std::uint_least32_t {
    return *raw_fd_;
  }

protected:
  std::optional<std::uint_least32_t> raw_fd_;
};

class splice_awaiter {
public:
  splice_awaiter(const std::uint_least32_t raw_fd_in,
                 const std::uint_least32_t raw_fd_out,
                 const std::uint_least32_t len) noexcept
      : raw_fd_in_{raw_fd_in}, raw_fd_out_{raw_fd_out}, len_{len} {}

  [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

  auto await_suspend(const std::coroutine_handle<> coroutine) noexcept -> void {
    sqe_data_.coroutine = coroutine.address();
    io_uring_context::get_instance().submit_splice_request(
        &sqe_data_, raw_fd_in_, raw_fd_out_, len_);
  }

  [[nodiscard]] auto await_resume() const noexcept
      -> std::expected<std::uint_least32_t, std::uint_least16_t> {
    return decode(sqe_data_.cqe_res);
  }

private:
  sqe_data sqe_data_;
  const std::uint_least32_t raw_fd_in_;
  const std::uint_least32_t raw_fd_out_;
  const std::uint_least32_t len_;
};

static_assert(awaiter<splice_awaiter>);

export [[nodiscard]] auto open(const std::string_view path) noexcept
    -> std::expected<file, std::uint_least16_t> {
  return decode(::open(path.data(), O_RDONLY))
      .transform([](const std::uint_least32_t raw_fd) static noexcept -> file {
        return file{static_cast<std::uint_least32_t>(raw_fd)};
      });
}

export [[nodiscard]] auto
pipe() noexcept -> std::expected<std::tuple<file, file>, std::uint_least16_t> {
  std::array<std::int_least32_t, 2> pipe_fd;
  return decode_void(::pipe(pipe_fd.data())).transform([pipe_fd]() {
    return std::tuple<file, file>{
        file{static_cast<std::uint_least32_t>(pipe_fd[0])},
        file{static_cast<std::uint_least32_t>(pipe_fd[1])}};
  });
};

export [[nodiscard]] auto splice(const file &in, const file &out,
                                 const std::uint_least32_t len) noexcept
    -> task<std::expected<std::uint_least32_t, std::uint_least16_t>> {
  std::expected<std::tuple<file, file>, std::uint_least16_t> pipe_result =
      pipe();
  if (!pipe_result.has_value()) {
    co_return std::unexpected{pipe_result.error()};
  }
  const auto &[read_pipe, write_pipe] = *pipe_result;

  std::uint_least32_t state = 0;
  while (state < len) {
    {
      auto splice_result = co_await splice_awaiter{
          in.get_raw_fd(), write_pipe.get_raw_fd(), len};
      if (!splice_result.has_value()) {
        co_return std::unexpected{splice_result.error()};
      }
    }
    {
      {
        auto splice_result = co_await splice_awaiter{read_pipe.get_raw_fd(),
                                                     out.get_raw_fd(), len};
        if (!splice_result.has_value()) {
          co_return std::unexpected{splice_result.error()};
        }
        state += *splice_result;
      }
    }
  }
  co_return state;
}

} // namespace co_uring_http
