export module co_uring_http.io:error;

import std;

namespace co_uring_http {

export [[nodiscard]] constexpr std::expected<std::uint_least32_t,
                                             std::uint_least16_t>
decode(const std::int_least32_t result) noexcept {
  if (result >= 0) {
    return static_cast<std::uint_least32_t>(result);
  }
  return std::unexpected{static_cast<std::uint_least16_t>(-result)};
}

export [[nodiscard]] constexpr std::expected<void, std::uint_least16_t>
decode_void(const std::int_least32_t result) noexcept {
  return decode(result).transform(
      [](const std::uint_least32_t) static noexcept -> void {});
}

template <typename T, template <typename...> typename Template>
constexpr bool specialization_of = false;

template <template <typename...> typename Template, typename... Args>
constexpr bool specialization_of<Template<Args...>, Template> = true;

export template <typename T>
  requires(specialization_of<std::remove_cvref_t<T>, std::expected>)
[[nodiscard]] auto
unwrap(T &&expected,
       const std::source_location source_location =
           std::source_location::current()) noexcept -> decltype(auto) {
  if (expected.has_value()) {
    return *std::forward<T>(expected);
  } else {
    std::println("thread '{}' panicked at {}:{} ({}):",
                 std::this_thread::get_id(), source_location.file_name(),
                 source_location.line(), source_location.function_name());
    std::println("called `unwrap()` on an `unexpected` value: {}",
                 expected.error());
    std::terminate();
  }
}

} // namespace co_uring_http
