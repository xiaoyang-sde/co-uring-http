#include <optional>
#include <stdexcept>
#include <unistd.h>

#include "file_descriptor.hpp"

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

auto file_descriptor::operator=(file_descriptor &&other) noexcept
    -> file_descriptor & {
  if (this == std::addressof(other)) {
    return *this;
  }
  raw_file_descriptor_ =
      std::exchange(other.raw_file_descriptor_, std::nullopt);
  return *this;
}

auto file_descriptor::operator<=>(const file_descriptor &other) const
    -> std::strong_ordering {
  return raw_file_descriptor_.value() <=> other.raw_file_descriptor_.value();
}

auto file_descriptor::get_raw_file_descriptor() const -> int {
  return raw_file_descriptor_.value();
}

} // namespace co_uring_http
