#ifndef SYNC_WAIT_HPP
#define SYNC_WAIT_HPP

#include <task.hpp>

namespace co_uring_http {
template <typename T> class sync_wait_task_promise;

template <typename T> class [[nodiscard]] sync_wait_task {
public:
  using promise_type = sync_wait_task_promise<T>;

  explicit sync_wait_task(
      std::coroutine_handle<sync_wait_task_promise<T>> coroutine_handle
  ) noexcept
      : coroutine(coroutine_handle) {}

  ~sync_wait_task() {
    if (coroutine) {
      coroutine.destroy();
    }
  }

  auto get_value() const noexcept -> T {
    return sync_wait_task<T>::coroutine.promise().get_value();
  }

  auto wait() const noexcept -> void {
    coroutine.promise().get_atomic_flag().wait(false);
  }

protected:
  std::coroutine_handle<sync_wait_task_promise<T>> coroutine;
};

template <typename T> class sync_wait_task_promise_base {
public:
  auto initial_suspend() const noexcept -> std::suspend_never { return {}; }

  class final_awaitable {
  public:
    constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_resume() const noexcept -> void { return; }

    auto
    await_suspend(std::coroutine_handle<sync_wait_task_promise<T>> coroutine
    ) const noexcept -> void {
      std::atomic_flag &atomic_flag = coroutine.promise().get_atomic_flag();
      atomic_flag.test_and_set();
      atomic_flag.notify_all();
    }
  };

  auto final_suspend() const noexcept -> final_awaitable { return {}; }

  auto unhandled_exception() const noexcept -> void { std::terminate(); }

  auto get_atomic_flag() noexcept -> std::atomic_flag & { return atomic_flag; }

private:
  std::atomic_flag atomic_flag;
};

template <typename T>
class sync_wait_task_promise final : public sync_wait_task_promise_base<T> {
public:
  auto get_return_object() noexcept -> sync_wait_task<T> {
    return sync_wait_task<T>{
        std::coroutine_handle<sync_wait_task_promise<T>>::from_promise(*this)};
  }

  auto return_value(T &&return_value) noexcept -> void {
    value = std::forward<T>(return_value);
  }

  constexpr auto get_value() const noexcept -> T { return value; }

private:
  T value;
};

template <>
class sync_wait_task_promise<void> final
    : public sync_wait_task_promise_base<void> {
public:
  auto get_return_object() noexcept -> sync_wait_task<void> {
    return sync_wait_task<void>{
        std::coroutine_handle<sync_wait_task_promise>::from_promise(*this)};
  }

  auto return_void() noexcept -> void {}

  constexpr auto get_value() const noexcept -> void {}
};

template <typename T> auto sync_wait(task<T> &task) -> T {
  sync_wait_task<T> sync_wait_task_handle =
      ([&]() -> sync_wait_task<T> { co_return co_await task; })();
  sync_wait_task_handle.wait();
  return sync_wait_task_handle.get_value();
}

template <typename T> auto sync_wait(task<T> &&task) -> T {
  sync_wait_task<T> sync_wait_task_handle =
      ([&]() -> sync_wait_task<T> { co_return co_await task; })();
  sync_wait_task_handle.wait();
  return sync_wait_task_handle.get_value();
}
} // namespace co_uring_http

#endif
