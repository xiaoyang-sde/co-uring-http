#include "thread_pool.hpp"

co_uring_http::thread_pool::thread_pool(
    const std::size_t thread_count) noexcept {
  for (size_t _ = 0; _ < thread_count; ++_) {
    thread_list.emplace_back([&]() { thread_loop(); });
  }
};

co_uring_http::thread_pool::~thread_pool() noexcept {
  stop_source.request_stop();
  condition_variable.notify_all();
};

co_uring_http::thread_pool::schedule_awaiter::schedule_awaiter(
    class thread_pool &thread_pool)
    : thread_pool{thread_pool} {}

auto co_uring_http::thread_pool::schedule_awaiter::await_suspend(
    std::coroutine_handle<> handle) const noexcept -> void {
  thread_pool.enqueue(handle);
};

auto co_uring_http::thread_pool::schedule() -> schedule_awaiter {
  return schedule_awaiter{*this};
}

auto co_uring_http::thread_pool::thread_loop() -> void {
  while (!stop_source.stop_requested()) {
    std::unique_lock lock(mutex);
    condition_variable.wait(lock, [this]() {
      return stop_source.stop_requested() || !coroutine_queue.empty();
    });
    if (stop_source.stop_requested()) {
      break;
    }

    const std::coroutine_handle<> coroutine_handle = coroutine_queue.front();
    coroutine_queue.pop();
    lock.unlock();

    coroutine_handle.resume();
  }
}

auto co_uring_http::thread_pool::enqueue(std::coroutine_handle<> handle)
    -> void {
  std::unique_lock lock(mutex);
  coroutine_queue.emplace(handle);
  condition_variable.notify_one();
}

auto co_uring_http::thread_pool::size() const noexcept -> size_t {
  return thread_list.size();
}
