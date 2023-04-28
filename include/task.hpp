#ifndef TASK_HPP
#define TASK_HPP

#include <coroutine>
#include <memory>

namespace co_uring_http {
template <typename T> class task_promise;

template <typename T = void> class [[nodiscard]] task {
public:
  using promise_type = task_promise<T>;

  explicit task(std::coroutine_handle<task_promise<T>> coroutine_handle
  ) noexcept
      : coroutine_(coroutine_handle) {}

  ~task() noexcept {
    if (coroutine_) {
      if (coroutine_.done()) {
        coroutine_.destroy();
      } else {
        coroutine_.promise().set_detached_coroutine(coroutine_);
      }
    }
  }

  task(task &&other) noexcept
      : coroutine_{std::exchange(other.coroutine_, nullptr)} {}

  auto operator=(task &&other) noexcept -> task & {
    if (this == std::addressof(other)) {
      return *this;
    }

    if (coroutine_) {
      coroutine_.destroy();
    }

    coroutine_ = std::exchange(other.coroutine_, nullptr);
    return *this;
  }

  task(const task &other) = delete;

  auto operator=(const task &other) -> task & = delete;

  class task_awaiter {
  public:
    explicit task_awaiter(std::coroutine_handle<task_promise<T>> coroutine)
        : coroutine_{coroutine} {}

    constexpr auto await_ready() const noexcept -> bool {
      return coroutine_ == nullptr || coroutine_.done();
    }

    constexpr auto await_resume() const noexcept -> T {
      return coroutine_.promise().get_return_value();
    }

    auto await_suspend(std::coroutine_handle<> calling_coroutine) const noexcept
        -> std::coroutine_handle<> {
      coroutine_.promise().set_calling_coroutine(calling_coroutine);
      return coroutine_;
    }

  private:
    std::coroutine_handle<task_promise<T>> coroutine_;
  };

  auto operator co_await() noexcept -> task_awaiter {
    return task_awaiter(coroutine_);
  }

  auto resume() noexcept -> void {
    if (coroutine_ == nullptr || coroutine_.done()) {
      return;
    }
    coroutine_.resume();
  }

  auto detach() noexcept {
    coroutine_.promise().set_detached_coroutine(coroutine_);
    coroutine_ = nullptr;
  }

private:
  std::coroutine_handle<task_promise<T>> coroutine_ = nullptr;
};

template <typename T> class task_promise_base {
public:
  class final_awaiter {
  public:
    final_awaiter(std::coroutine_handle<> detached_coroutine)
        : detached_coroutine_{detached_coroutine} {};

    constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_resume() const noexcept -> void { return; }

    auto await_suspend(std::coroutine_handle<task_promise<T>> coroutine
    ) const noexcept -> std::coroutine_handle<> {
      if (coroutine.promise().calling_coroutine_) {
        return coroutine.promise().calling_coroutine_;
      }
      if (detached_coroutine_) {
        detached_coroutine_.destroy();
      }
      return std::noop_coroutine();
    }

  private:
    std::coroutine_handle<> detached_coroutine_;
  };

  auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  auto final_suspend() noexcept -> final_awaiter {
    return {detached_coroutine_};
  }

  auto unhandled_exception() -> void { std::terminate(); }

  auto set_calling_coroutine(std::coroutine_handle<> coroutine) -> void {
    calling_coroutine_ = coroutine;
  }

  auto set_detached_coroutine(std::coroutine_handle<> coroutine) -> void {
    detached_coroutine_ = coroutine;
  }

private:
  std::coroutine_handle<> calling_coroutine_ = nullptr;
  std::coroutine_handle<> detached_coroutine_ = nullptr;
};

template <typename T> class task_promise final : public task_promise_base<T> {
public:
  auto get_return_object() noexcept -> task<T> {
    return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
  }

  auto return_value(T &&return_value) noexcept -> void {
    return_value_ = std::forward<T>(return_value);
  }

  constexpr auto get_return_value() const noexcept -> T {
    return return_value_;
  }

private:
  T return_value_;
};

template <> class task_promise<void> final : public task_promise_base<void> {
public:
  auto get_return_object() noexcept -> task<void> {
    return task<void>{std::coroutine_handle<task_promise>::from_promise(*this)};
  };
  auto return_void() noexcept -> void {}

  constexpr auto get_return_value() const noexcept -> void {}
};
} // namespace co_uring_http

#endif
