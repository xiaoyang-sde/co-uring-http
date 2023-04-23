#ifndef SYNC_WAIT_HPP
#define SYNC_WAIT_HPP

#include <atomic>
#include <coroutine>
#include <task.hpp>

namespace co_uring_http {
class sync_wait_task_promise;

class [[nodiscard]] sync_wait_task {
public:
  using promise_type = sync_wait_task_promise;

  explicit sync_wait_task(
      std::coroutine_handle<sync_wait_task_promise> coroutine_handle) noexcept;

  ~sync_wait_task();

  auto wait() const noexcept -> void;

private:
  std::coroutine_handle<sync_wait_task_promise> coroutine;
};

class sync_wait_task_promise {
public:
  auto get_return_object() noexcept -> sync_wait_task;

  auto initial_suspend() const noexcept -> std::suspend_never;

  class final_awaitable {
  public:
    constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_resume() const noexcept -> void { return; }

    auto await_suspend(std::coroutine_handle<sync_wait_task_promise> coroutine)
        const noexcept -> void;
  };

  auto final_suspend() const noexcept -> final_awaitable;

  auto unhandled_exception() const noexcept -> void;

  auto get_atomic_flag() noexcept -> std::atomic_flag &;

private:
  std::atomic_flag atomic_flag;
};

template <typename T>
auto build_sync_wait_task(task<T> &task) -> sync_wait_task {
  co_await task;
}

template <typename T> inline void sync_wait(task<T> &task) {
  build_sync_wait_task(task).wait();
}
} // namespace co_uring_http

#endif
