#include "thread_pool.hpp"

namespace co_uring_http {
thread_pool::thread_pool(const std::size_t thread_count) {
  for (size_t _ = 0; _ < thread_count; ++_) {
    thread_list_.emplace_back([&]() { thread_loop(); });
  }
}

thread_pool::~thread_pool() {
  stop_source_.request_stop();
  condition_variable_.notify_all();
}

thread_pool::schedule_awaiter::schedule_awaiter(class thread_pool &thread_pool)
    : thread_pool_{thread_pool} {}

auto thread_pool::schedule_awaiter::await_suspend(std::coroutine_handle<> handle) const noexcept
    -> void {
  thread_pool_.enqueue(handle);
}

auto thread_pool::schedule() -> schedule_awaiter { return schedule_awaiter{*this}; }

auto thread_pool::size() const noexcept -> size_t { return thread_list_.size(); }

auto thread_pool::thread_loop() -> void {
  while (!stop_source_.stop_requested()) {
    std::unique_lock lock(mutex_);
    condition_variable_.wait(lock, [this]() {
      return stop_source_.stop_requested() || !coroutine_queue_.empty();
    });
    if (stop_source_.stop_requested()) {
      break;
    }

    const std::coroutine_handle<> coroutine = coroutine_queue_.front();
    coroutine_queue_.pop();
    lock.unlock();

    coroutine.resume();
  }
}

auto thread_pool::enqueue(std::coroutine_handle<> coroutine) -> void {
  std::unique_lock lock(mutex_);
  coroutine_queue_.emplace(coroutine);
  condition_variable_.notify_one();
}
} // namespace co_uring_http
