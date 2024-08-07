export module co_uring_http.common:deferred_initialization;

import std;

namespace co_uring_http {

export template <typename T> class deferred_initialization {
public:
  deferred_initialization() noexcept = default;

  template <typename U>
    requires(std::is_constructible_v<T, U>)
  deferred_initialization(U &&value) noexcept(
      std::is_nothrow_constructible_v<T, U>) {
    std::construct_at(std::addressof(storage_.value), std::forward<U>(value));
  }

  template <typename U>
    requires(std::is_constructible_v<T, U>)
  auto operator=(U &&value) noexcept(std::is_nothrow_constructible_v<T, U>)
      -> deferred_initialization & {
    std::construct_at(std::addressof(storage_.value), std::forward<U>(value));
    return *this;
  }

  template <typename... Args>
    requires(std::is_constructible_v<T, Args...>)
  auto emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) -> void {
    std::construct_at(std::addressof(storage_.value),
                      std::forward<Args>(args)...);
  }

  template <typename Self>
  [[nodiscard]] auto operator*(this Self &&self) noexcept -> decltype(auto) {
    return std::forward_like<Self>(self.storage_.value);
  }

  template <typename Self>
    requires std::is_lvalue_reference_v<Self>
  [[nodiscard]] auto operator->(this Self &&self) noexcept -> T * {
    return std::addressof(self.storage_.value);
  }

private:
  union storage {
  public:
    storage() noexcept {}

    ~storage() noexcept(std::is_nothrow_destructible_v<T>)
      requires(std::is_trivially_destructible_v<T>)
    = default;

    ~storage() noexcept(std::is_nothrow_destructible_v<T>) { value.~T(); }

    T value;
  } storage_;
};

} // namespace co_uring_http
