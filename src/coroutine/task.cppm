export module co_uring_http.coroutine:task;

import std;

import co_uring_http.common;

import :trait;

namespace co_uring_http {

template <typename T = void> class task_promise;

export template <typename T = void> class task {
public:
  using promise_type = task_promise<T>;

  task(task &&other) noexcept
      : coroutine_{std::exchange(other.coroutine_, nullptr)} {}

  ~task() {
    if (coroutine_ != nullptr) {
      if (coroutine_.done()) {
        coroutine_.destroy();
      } else {
        coroutine_.promise().set_detached_flag(true);
      }
    }
  }

  class task_awaiter {
  public:
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    [[nodiscard]] auto await_suspend(const std::coroutine_handle<> continuation)
        const noexcept -> std::coroutine_handle<> {
      coroutine_.promise().set_continuation(continuation);
      return coroutine_;
    }

    [[nodiscard]] auto await_resume() const noexcept -> decltype(auto) {
      if constexpr (!std::same_as<T, void>) {
        return coroutine_.promise().get_return_value();
      }
    }

  private:
    friend task;

    explicit task_awaiter(
        const std::coroutine_handle<task_promise<T>> coroutine) noexcept
        : coroutine_{coroutine} {}

    const std::coroutine_handle<task_promise<T>> coroutine_;
  };

  static_assert(awaiter<task_awaiter>);

  [[nodiscard]] auto operator co_await() noexcept -> task_awaiter {
    return task_awaiter{coroutine_};
  }

  auto resume() const noexcept -> void {
    if (coroutine_ != nullptr) {
      coroutine_.resume();
    }
  }

  auto detach() noexcept -> void {
    coroutine_.promise().set_detached_flag(true);
  }

private:
  friend promise_type;

  explicit task(const std::coroutine_handle<task_promise<T>> coroutine) noexcept
      : coroutine_{coroutine} {}

  std::coroutine_handle<task_promise<T>> coroutine_;
};

static_assert(awaitable<task<>>);

template <typename T = void> class task_promise_base {
public:
  class final_awaiter {
  public:
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto await_resume() const noexcept -> void {}

    [[nodiscard]] auto
    await_suspend(const std::coroutine_handle<task_promise<T>> coroutine)
        const noexcept -> std::coroutine_handle<> {
      if (coroutine.promise().get_detached_flag()) {
        coroutine.destroy();
        return std::noop_coroutine();
      }
      return coroutine.promise().get_continuation();
    }
  };

  static_assert(awaiter<final_awaiter>);

  task_promise_base() noexcept
      : continuation_{std::noop_coroutine()}, detached_flag_{false} {}

  [[nodiscard]] auto initial_suspend() const noexcept -> std::suspend_always {
    return std::suspend_always{};
  }

  [[nodiscard]] auto final_suspend() const noexcept -> final_awaiter {
    return final_awaiter{};
  }

  [[noreturn]] auto unhandled_exception() const noexcept -> void {
    std::terminate();
  }

  [[nodiscard]] auto
  get_continuation() const noexcept -> std::coroutine_handle<> {
    return continuation_;
  }

  auto set_continuation(const std::coroutine_handle<> continuation) noexcept
      -> void {
    continuation_ = continuation;
  }

  [[nodiscard]] auto get_detached_flag() const noexcept -> bool {
    return detached_flag_;
  }

  auto set_detached_flag(const bool detached_flag) noexcept -> void {
    detached_flag_ = detached_flag;
  }

private:
  std::coroutine_handle<> continuation_;
  bool detached_flag_;
};

template <> class task_promise<void> final : public task_promise_base<void> {
public:
  [[nodiscard]] auto get_return_object() noexcept -> task<void> {
    return task<void>{std::coroutine_handle<task_promise>::from_promise(*this)};
  };

  auto return_void() const noexcept -> void {}
};

template <typename T> class task_promise final : public task_promise_base<T> {
public:
  [[nodiscard]] auto get_return_object() noexcept -> task<T> {
    return task<T>{std::coroutine_handle<task_promise>::from_promise(*this)};
  }

  template <typename U>
    requires std::convertible_to<U &&, T>
  auto return_value(U &&return_value) noexcept(
      std::is_nothrow_convertible_v<U &&, T>) -> void {
    return_value_.emplace(std::forward<U>(return_value));
  }

  template <typename Self>
  [[nodiscard]]
  auto get_return_value(this Self &&self) noexcept -> decltype(auto) {
    return *(std::forward<Self>(self).return_value_);
  }

private:
  deferred_initialization<T> return_value_;
};

}; // namespace co_uring_http
