export module co_uring_http.coroutine:wait;

import std;

import co_uring_http.common;

import :task;
import :trait;

namespace co_uring_http {

template <typename T = void> class wait_task_promise;

template <typename T = void> class wait_task {
public:
  using promise_type = wait_task_promise<T>;

  ~wait_task() { coroutine_.destroy(); }

  auto wait() const noexcept -> void {
    coroutine_.promise().get_stop_token().wait(false,
                                               std::memory_order_acquire);
  }

  template <typename Self>
  [[nodiscard]]
  auto get_return_value(this Self &&self) noexcept -> decltype(auto) {
    return std::forward<Self>(self).coroutine_.promise().get_return_value();
  }

private:
  friend promise_type;

  explicit wait_task(
      const std::coroutine_handle<wait_task_promise<T>> coroutine) noexcept
      : coroutine_{coroutine} {}

  const std::coroutine_handle<wait_task_promise<T>> coroutine_;
};

template <typename T = void> class wait_task_promise_base {
public:
  class final_awaiter {
  public:
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto await_resume() const noexcept -> void {}

    auto await_suspend(const std::coroutine_handle<wait_task_promise<T>>
                           coroutine) const noexcept -> void {
      std::atomic_flag &stop_token_ = coroutine.promise().get_stop_token();
      stop_token_.test_and_set(std::memory_order_release);
      stop_token_.notify_one();
    }
  };

  static_assert(awaiter<final_awaiter>);

  [[nodiscard]] auto initial_suspend() const noexcept -> std::suspend_never {
    return std::suspend_never{};
  }

  [[nodiscard]] auto final_suspend() const noexcept -> final_awaiter {
    return final_awaiter{};
  }

  [[noreturn]] auto unhandled_exception() const noexcept -> void {
    std::terminate();
  }

  auto get_stop_token() noexcept -> std::atomic_flag & { return stop_token_; }

private:
  std::atomic_flag stop_token_;
};

template <>
class wait_task_promise<void> final : public wait_task_promise_base<void> {
public:
  [[nodiscard]] auto get_return_object() noexcept -> wait_task<void> {
    return wait_task<void>{
        std::coroutine_handle<wait_task_promise>::from_promise(*this)};
  };

  auto return_void() const noexcept -> void {}
};

template <typename T>
class wait_task_promise final : public wait_task_promise_base<T> {
public:
  [[nodiscard]] auto get_return_object() noexcept -> wait_task<T> {
    return wait_task{
        std::coroutine_handle<wait_task_promise>::from_promise(*this)};
  }

  template <typename U>
    requires std::convertible_to<U &&, T>
  auto return_value(U &&return_value) noexcept(
      std::is_nothrow_convertible_v<U &&, T>) -> void {
    return_value_.emplace(std::forward<U>(return_value));
  }

  template <typename Self>
  [[nodiscard]]
  auto get_return_value(this Self &&self) noexcept -> decltype(auto) {
    return *(std::forward<Self>(self).return_value_);
  }

private:
  deferred_initialization<T> return_value_;
};

export template <typename Awaitable>
  requires awaitable<Awaitable>
auto wait(Awaitable &&awaitable) {
  auto wait_task_promise =
      [&awaitable]() noexcept -> wait_task<await_result_t<Awaitable>> {
    co_return co_await std::forward<Awaitable>(awaitable);
  }();
  wait_task_promise.wait();

  if constexpr (!std::same_as<await_result_t<Awaitable>, void>) {
    return wait_task_promise.get_return_value();
  }
}

}; // namespace co_uring_http
