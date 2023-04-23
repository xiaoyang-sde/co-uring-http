#include "sync_wait.hpp"

co_uring_http::sync_wait_task::sync_wait_task(
    std::coroutine_handle<sync_wait_task_promise> coroutine_handle) noexcept
    : coroutine(coroutine_handle) {}

co_uring_http::sync_wait_task::~sync_wait_task() {
  if (coroutine) {
    coroutine.destroy();
  }
}

auto co_uring_http::sync_wait_task::wait() const noexcept -> void {
  coroutine.promise().get_atomic_flag().wait(false);
}

auto co_uring_http::sync_wait_task_promise::get_return_object() noexcept
    -> sync_wait_task {
  return sync_wait_task{
      std::coroutine_handle<sync_wait_task_promise>::from_promise(*this)};
};

auto co_uring_http::sync_wait_task_promise::initial_suspend() const noexcept
    -> std::suspend_never {
  return {};
}

auto co_uring_http::sync_wait_task_promise::final_suspend() const noexcept
    -> final_awaitable {
  return {};
}

auto co_uring_http::sync_wait_task_promise::unhandled_exception() const noexcept
    -> void {
  std::terminate();
}

auto co_uring_http::sync_wait_task_promise::get_atomic_flag() noexcept
    -> std::atomic_flag & {
  return atomic_flag;
}

auto co_uring_http::sync_wait_task_promise::final_awaitable::await_suspend(
    std::coroutine_handle<sync_wait_task_promise> coroutine) const noexcept
    -> void {
  std::atomic_flag &atomic_flag = coroutine.promise().atomic_flag;
  atomic_flag.test_and_set();
  atomic_flag.notify_all();
};
