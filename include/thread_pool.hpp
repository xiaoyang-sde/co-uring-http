#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <coroutine>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

namespace co_uring_http {
class thread_pool {
public:
  explicit thread_pool(const std::size_t thread_count) noexcept;
  ~thread_pool() noexcept;

  class schedule_awaiter {
  public:
    schedule_awaiter(thread_pool &thread_pool);

    constexpr auto await_ready() const noexcept -> bool { return false; };
    constexpr auto await_resume() const noexcept -> void { return; };
    auto await_suspend(std::coroutine_handle<> handle) const noexcept -> void;

  private:
    thread_pool &thread_pool;
  };

  auto schedule() -> schedule_awaiter;

private:
  std::mutex mutex;
  std::condition_variable condition_variable;

  std::stop_source stop_source;
  std::list<std::jthread> thread_list;
  std::queue<std::coroutine_handle<>> coroutine_queue;

  auto thread_loop() -> void;

  auto enqueue(std::coroutine_handle<> handle) -> void;
};
} // namespace co_uring_http

#endif
