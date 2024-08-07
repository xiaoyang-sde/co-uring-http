export module co_uring_http.coroutine:thread_pool;

import std;

import :trait;

namespace co_uring_http {

export class thread_pool {
public:
  explicit thread_pool(const std::uint_least32_t thread_count =
                           std::thread::hardware_concurrency())
      : stop_token_{false},
        thread_list_{std::views::iota(0) | std::views::take(thread_count) |
                     std::views::transform(
                         [this](const std::uint_least32_t) -> std::thread {
                           return std::thread{
                               [this]() noexcept -> void { thread_loop(); }};
                         }) |
                     std::ranges::to<std::vector<std::thread>>()} {}

  ~thread_pool() {
    {
      const std::unique_lock _(mutex_);
      stop_token_ = true;
      condition_variable_.notify_all();
    }

    std::ranges::for_each(thread_list_, &std::thread::join);
  }

  class schedule_awaiter {
  public:
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto await_suspend(const std::coroutine_handle<> coroutine) const noexcept
        -> void {
      thread_pool_.enqueue(coroutine);
    }

    auto await_resume() const noexcept -> void {}

  private:
    friend thread_pool;

    explicit schedule_awaiter(thread_pool &thread_pool) noexcept
        : thread_pool_{thread_pool} {}

    thread_pool &thread_pool_;
  };

  static_assert(awaitable<schedule_awaiter>);

  [[nodiscard]] auto schedule() noexcept -> schedule_awaiter {
    return schedule_awaiter{*this};
  }

  [[nodiscard]] auto size() const noexcept -> std::uint_least32_t {
    return thread_list_.size();
  }

private:
  auto enqueue(const std::coroutine_handle<> coroutine) -> void {
    const std::unique_lock lock(mutex_);
    coroutine_queue_.emplace_back(coroutine);
    condition_variable_.notify_one();
  }

  auto thread_loop() -> void {
    while (!stop_token_) {
      std::unique_lock lock(mutex_);
      condition_variable_.wait(lock, [this]() noexcept -> bool {
        return stop_token_ || !coroutine_queue_.empty();
      });
      if (stop_token_) {
        break;
      }

      const std::coroutine_handle<> coroutine = coroutine_queue_.back();
      coroutine_queue_.pop_back();
      lock.unlock();

      coroutine.resume();
    }
  }

  std::mutex mutex_;
  std::condition_variable condition_variable_;
  bool stop_token_;

  std::vector<std::coroutine_handle<>> coroutine_queue_;
  std::vector<std::thread> thread_list_;
};

} // namespace co_uring_http
