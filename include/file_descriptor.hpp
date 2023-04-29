#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <compare>
#include <optional>

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
} // namespace co_uring_http

#endif
