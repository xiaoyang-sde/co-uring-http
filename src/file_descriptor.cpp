#include <optional>
#include <stdexcept>
#include <unistd.h>

#include "file_descriptor.hpp"

namespace co_uring_http {
file_descriptor::file_descriptor() {}

file_descriptor::file_descriptor(const int fd) : fd_{fd} {}

file_descriptor::~file_descriptor() {
  if (fd_.has_value()) {
    close(fd_.value());
  }
}

file_descriptor::file_descriptor(file_descriptor &&other) noexcept
    : fd_{other.fd_} {
  other.fd_ = std::nullopt;
}

auto file_descriptor::operator=(file_descriptor &&other) noexcept
    -> file_descriptor & {
  if (this == std::addressof(other)) {
    return *this;
  }
  fd_ = std::exchange(other.fd_, std::nullopt);
  return *this;
}

auto file_descriptor::operator<=>(const file_descriptor &other) const
    -> std::strong_ordering {
  return fd_.value() <=> other.fd_.value();
}

auto file_descriptor::get_fd() const -> int { return fd_.value(); }

} // namespace co_uring_http
