#ifndef SYNC_WAIT_HPP
#define SYNC_WAIT_HPP

#include <algorithm>
#include <atomic>
#include <optional>
#include <vector>

#include "task.hpp"

namespace co_uring_http {
template <typename T> class sync_wait_task_promise;

template <typename T> class [[nodiscard]] sync_wait_task {
public:
  using promise_type = sync_wait_task_promise<T>;

  explicit sync_wait_task(std::coroutine_handle<sync_wait_task_promise<T>> coroutine_handle
  ) noexcept
      : coroutine_(coroutine_handle) {}

  ~sync_wait_task() {
    if (coroutine_) {
      coroutine_.destroy();
    }
  }

  [[nodiscard]] auto get_return_value() const noexcept -> T {
    return coroutine_.promise().get_return_value();
  }

  auto wait() const noexcept -> void {
    coroutine_.promise().get_atomic_flag().wait(false, std::memory_order_acquire);
  }

private:
  std::coroutine_handle<sync_wait_task_promise<T>> coroutine_;
};

template <typename T> class sync_wait_task_promise_base {
public:
  [[nodiscard]] auto initial_suspend() const noexcept -> std::suspend_never { return {}; }

  class final_awaiter {
  public:
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto await_resume() const noexcept -> void {}

    auto await_suspend(std::coroutine_handle<sync_wait_task_promise<T>> coroutine) const noexcept
        -> void {
      std::atomic_flag &atomic_flag = coroutine.promise().get_atomic_flag();
      atomic_flag.test_and_set(std::memory_order_release);
      atomic_flag.notify_all();
    }
  };

  [[nodiscard]] auto final_suspend() const noexcept -> final_awaiter { return {}; }

  auto unhandled_exception() const noexcept -> void { std::terminate(); }

  auto get_atomic_flag() noexcept -> std::atomic_flag & { return atomic_flag_; }

private:
  std::atomic_flag atomic_flag_;
};

template <typename T> class sync_wait_task_promise final : public sync_wait_task_promise_base<T> {
public:
  auto get_return_object() noexcept -> sync_wait_task<T> {
    return sync_wait_task<T>{std::coroutine_handle<sync_wait_task_promise<T>>::from_promise(*this)};
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

template <> class sync_wait_task_promise<void> final : public sync_wait_task_promise_base<void> {
public:
  auto get_return_object() noexcept -> sync_wait_task<void> {
    return sync_wait_task<void>{std::coroutine_handle<sync_wait_task_promise>::from_promise(*this)};
  }

  auto return_void() noexcept -> void {}
};

template <typename T> auto sync_wait(task<T> &task) -> T {
  auto sync_wait_task_handle = ([&]() -> sync_wait_task<T> { co_return co_await task; })();
  sync_wait_task_handle.wait();

  if constexpr (!std::is_same_v<T, void>) {
    return sync_wait_task_handle.get_return_value();
  }
}

template <typename T> auto sync_wait(task<T> &&task) -> T {
  auto sync_wait_task_handle = ([&]() -> sync_wait_task<T> { co_return co_await task; })();
  sync_wait_task_handle.wait();

  if constexpr (!std::is_same_v<T, void>) {
    return sync_wait_task_handle.get_return_value();
  }
}

template <typename T>
auto sync_wait_all(std::vector<task<T>> &task_list)
    -> std::conditional_t<std::is_same_v<T, void>, void, std::vector<T>> {
  if constexpr (std::is_same_v<T, void>) {
    for (auto &task : task_list) {
      sync_wait(task);
    }
  } else {
    std::vector<T> return_value_list;
    return_value_list.reserve(task_list.size());

    std::transform(
        task_list.begin(), task_list.end(), std::back_inserter(return_value_list),
        [](task<T> &task) -> T { return sync_wait(task); }
    );
    return return_value_list;
  }
}
} // namespace co_uring_http

#endif
