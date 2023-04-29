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
  explicit thread_pool(std::size_t thread_count);
  ~thread_pool();

  class schedule_awaiter {
  public:
    explicit schedule_awaiter(thread_pool &thread_pool);

    [[nodiscard]] constexpr auto await_ready() noexcept -> bool { return false; }
    constexpr auto await_resume() const noexcept -> void {}
    auto await_suspend(std::coroutine_handle<> handle) const noexcept -> void;

  private:
    thread_pool &thread_pool_;
  };

  auto schedule() -> schedule_awaiter;
  [[nodiscard]] auto size() const noexcept -> size_t;

private:
  std::stop_source stop_source_;
  std::list<std::jthread> thread_list_;

  std::mutex mutex_;
  std::condition_variable condition_variable_;
  std::queue<std::coroutine_handle<>> coroutine_queue_;

  auto thread_loop() -> void;
  auto enqueue(std::coroutine_handle<> coroutine) -> void;
};
} // namespace co_uring_http

#endif
