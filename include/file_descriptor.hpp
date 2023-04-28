#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <compare>
#include <optional>

namespace co_uring_http {
class file_descriptor {
public:
  file_descriptor();

  explicit file_descriptor(const int fd);

  ~file_descriptor();

  file_descriptor(file_descriptor &&other) noexcept;

  auto operator=(file_descriptor &&other) noexcept -> file_descriptor &;

  file_descriptor(const file_descriptor &other) = delete;

  auto operator=(const file_descriptor &other) -> file_descriptor & = delete;

  auto operator<=>(const file_descriptor &other) const -> std::strong_ordering;

  auto get_fd() const -> int;

protected:
  std::optional<int> fd_;
};
} // namespace co_uring_http

#endif
