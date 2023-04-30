#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <unistd.h>

#include <compare>
#include <coroutine>
#include <filesystem>
#include <optional>
#include <tuple>

#include "io_uring.hpp"
#include "task.hpp"

namespace co_uring_http {
class file_descriptor {
public:
  file_descriptor();

  explicit file_descriptor(int raw_file_descriptor);

  ~file_descriptor();

  file_descriptor(file_descriptor &&other) noexcept;

  auto operator=(file_descriptor &&other) noexcept -> file_descriptor &;

  file_descriptor(const file_descriptor &other) = delete;

  auto operator=(const file_descriptor &other) -> file_descriptor & = delete;

  auto operator<=>(const file_descriptor &other) const -> std::strong_ordering;

  [[nodiscard]] auto get_raw_file_descriptor() const -> int;

protected:
  std::optional<int> raw_file_descriptor_;
};

class splice_awaiter {
public:
  splice_awaiter(int raw_file_descriptor_in, int raw_file_descriptor_out, size_t length);

  [[nodiscard]] auto await_ready() const -> bool;
  auto await_suspend(std::coroutine_handle<> coroutine) -> void;
  [[nodiscard]] auto await_resume() const -> ssize_t;

private:
  const int raw_file_descriptor_in_;
  const int raw_file_descriptor_out_;
  const size_t length_;
  sqe_data sqe_data_;
};

auto splice(
    const file_descriptor &file_descriptor_in, const file_descriptor &file_descriptor_out,
    const size_t length
) -> task<ssize_t>;

auto pipe() -> std::tuple<file_descriptor, file_descriptor>;

auto open(const std::filesystem::path &path) -> file_descriptor;
} // namespace co_uring_http

#endif
