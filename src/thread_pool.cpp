#include "thread_pool.hpp"

namespace co_uring_http {
thread_pool::thread_pool(const std::size_t thread_count) {
  for (size_t _ = 0; _ < thread_count; ++_) {
    thread_list.emplace_back([&]() { thread_loop(); });
  }
}

thread_pool::~thread_pool() {
  stop_source.request_stop();
  condition_variable.notify_all();
}

thread_pool::schedule_awaitable::schedule_awaitable(
    class thread_pool &thread_pool
)
    : thread_pool{thread_pool} {}

auto thread_pool::schedule_awaitable::await_suspend(
    std::coroutine_handle<> handle
) const noexcept -> void {
  thread_pool.enqueue(handle);
}

auto thread_pool::schedule() -> schedule_awaitable {
  return schedule_awaitable{*this};
}

auto thread_pool::size() const noexcept -> size_t { return thread_list.size(); }

auto thread_pool::thread_loop() -> void {
  while (!stop_source.stop_requested()) {
    std::unique_lock lock(mutex);
    condition_variable.wait(lock, [this]() {
      return stop_source.stop_requested() || !coroutine_queue.empty();
    });
    if (stop_source.stop_requested()) {
      break;
    }

    const std::coroutine_handle<> coroutine = coroutine_queue.front();
    coroutine_queue.pop();
    lock.unlock();

    coroutine.resume();
  }
}

auto thread_pool::enqueue(std::coroutine_handle<> coroutine) -> void {
  std::unique_lock lock(mutex);
  coroutine_queue.emplace(coroutine);
  condition_variable.notify_one();
}
} // namespace co_uring_http
