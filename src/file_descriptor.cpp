#include <optional>
#include <stdexcept>
#include <unistd.h>

#include "file_descriptor.hpp"

namespace co_uring_http {
file_descriptor::file_descriptor() {}

file_descriptor::file_descriptor(const int fd) : fd{fd} {}

file_descriptor::~file_descriptor() {
  if (fd.has_value()) {
    close(fd.value());
  }
}

file_descriptor::file_descriptor(file_descriptor &&other) noexcept
    : fd{other.fd} {
  other.fd = std::nullopt;
}

auto file_descriptor::operator=(file_descriptor &&other) noexcept
    -> file_descriptor & {
  if (this == std::addressof(other)) {
    return *this;
  }
  fd = std::exchange(other.fd, std::nullopt);
  return *this;
}

auto file_descriptor::operator<=>(const file_descriptor &other) const
    -> std::strong_ordering {
  return fd.value() <=> other.fd.value();
}

auto file_descriptor::get_fd() const -> int { return fd.value(); }

} // namespace co_uring_http
