#ifndef TASK_HPP
#define TASK_HPP

#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>

namespace co_uring_http {
template <typename T> class task_promise;

template <typename T = void> class [[nodiscard]] task {
public:
  using promise_type = task_promise<T>;

  explicit task(std::coroutine_handle<task_promise<T>> coroutine_handle) noexcept
      : coroutine_(coroutine_handle) {}

  ~task() noexcept {
    if (coroutine_) {
      if (coroutine_.done()) {
        coroutine_.destroy();
      } else {
        coroutine_.promise().get_detached_flag().test_and_set(std::memory_order_relaxed);
      }
    }
  }

  task(task &&other) noexcept : coroutine_{std::exchange(other.coroutine_, nullptr)} {}

  auto operator=(task &&other) noexcept -> task & = delete;

  task(const task &other) = delete;

  auto operator=(const task &other) -> task & = delete;

  class task_awaiter {
  public:
    explicit task_awaiter(std::coroutine_handle<task_promise<T>> coroutine)
        : coroutine_{coroutine} {}

    [[nodiscard]] auto await_ready() const noexcept -> bool {
      return coroutine_ == nullptr || coroutine_.done();
    }

    [[nodiscard]] auto await_resume() const noexcept -> decltype(auto) {
      if constexpr (!std::is_same_v<T, void>) {
        return coroutine_.promise().get_return_value();
      }
    }

    [[nodiscard]] auto await_suspend(std::coroutine_handle<> calling_coroutine) const noexcept
        -> std::coroutine_handle<> {
      coroutine_.promise().get_calling_coroutine() = calling_coroutine;
      return coroutine_;
    }

  private:
    std::coroutine_handle<task_promise<T>> coroutine_;
  };

  auto operator co_await() noexcept -> task_awaiter { return task_awaiter(coroutine_); }

  auto resume() const noexcept -> void {
    if (coroutine_ == nullptr || coroutine_.done()) {
      return;
    }
    coroutine_.resume();
  }

  auto detach() noexcept {
    coroutine_.promise().get_detached_flag().test_and_set(std::memory_order_relaxed);
    coroutine_ = nullptr;
  }

private:
  std::coroutine_handle<task_promise<T>> coroutine_ = nullptr;
};

template <typename T> class task_promise_base {
public:
  class final_awaiter {
  public:
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto await_resume() const noexcept -> void {}

    [[nodiscard]] auto await_suspend(std::coroutine_handle<task_promise<T>> coroutine
    ) const noexcept -> std::coroutine_handle<> {
      if (coroutine.promise().get_detached_flag().test(std::memory_order_relaxed)) {
        coroutine.destroy();
      }
      return coroutine.promise().get_calling_coroutine().value_or(std::noop_coroutine());
    }
  };

  [[nodiscard]] auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

  [[nodiscard]] auto final_suspend() const noexcept -> final_awaiter { return final_awaiter{}; }

  auto unhandled_exception() const noexcept -> void { std::terminate(); }

  [[nodiscard]] auto get_calling_coroutine() noexcept -> std::optional<std::coroutine_handle<>> & {
    return calling_coroutine_;
  }

  [[nodiscard]] auto get_detached_flag() noexcept -> std::atomic_flag & { return detached_flag_; }

private:
  std::optional<std::coroutine_handle<>> calling_coroutine_;
  std::atomic_flag detached_flag_;
};

template <typename T> class task_promise final : public task_promise_base<T> {
public:
  auto get_return_object() noexcept -> task<T> {
    return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
  }

  template <typename U>
    requires std::convertible_to<U &&, T>
  auto return_value(U &&return_value) noexcept(std::is_nothrow_constructible_v<T, U &&>) -> void {
    return_value_.emplace(std::forward<U>(return_value));
  }

  [[nodiscard]] auto get_return_value() & noexcept -> T & { return *return_value_; }

  [[nodiscard]] auto get_return_value() && noexcept -> T && { return std::move(*return_value_); }

private:
  std::optional<T> return_value_;
};

template <> class task_promise<void> final : public task_promise_base<void> {
public:
  [[nodiscard]] auto get_return_object() noexcept -> task<void> {
    return task<void>{std::coroutine_handle<task_promise>::from_promise(*this)};
  };

  auto return_void() const noexcept -> void {}
};
} // namespace co_uring_http

#endif
