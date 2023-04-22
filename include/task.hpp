#ifndef TASK_HPP
#define TASK_HPP

#include <coroutine>
#include <exception>
#include <memory>

namespace co_uring_http {
template <typename T> class task_promise;

template <typename T = void> class [[nodiscard]] task {
public:
  using promise_type = task_promise<T>;

  explicit task(
      std::coroutine_handle<task_promise<T>> coroutine_handle) noexcept
      : coroutine(coroutine_handle){};

  ~task() noexcept {
    if (coroutine) {
      coroutine.destroy();
    }
  };

  task(task &&other) noexcept : coroutine(other.coroutine) {
    other.coroutine = nullptr;
  };

  auto operator=(task &&other) noexcept -> task & {
    if (this == std::addressof(other)) {
      return *this;
    }

    if (coroutine) {
      coroutine.destroy();
    }
    coroutine = std::exchange(other.coroutine, nullptr);
    return *this;
  };

  task(const task &other) = delete;

  auto operator=(const task &other) -> task & = delete;

  class task_awaitable {
  public:
    task_awaitable(std::coroutine_handle<task_promise<T>> coroutine)
        : coroutine{coroutine} {}

    constexpr auto await_ready() const noexcept -> bool { return false; };

    template <typename U = T>
    constexpr auto await_resume() const noexcept -> U {
      return coroutine.promise().get_value();
    };

    template <> constexpr auto await_resume() const noexcept -> void{};

    auto await_suspend(std::coroutine_handle<> calling_coroutine) const noexcept
        -> std::coroutine_handle<> {
      coroutine.promise().set_calling_coroutine(calling_coroutine);
      return coroutine;
    };

  private:
    std::coroutine_handle<task_promise<T>> coroutine;
  };

  auto operator co_await() noexcept -> task_awaitable { return {coroutine}; }

private:
  std::coroutine_handle<task_promise<T>> coroutine;
};

template <typename T> class task_promise_base {
public:
  class final_awaitable {
  public:
    constexpr auto await_ready() const noexcept -> bool { return false; };
    constexpr auto await_resume() const noexcept -> void { return; };
    auto await_suspend(std::coroutine_handle<task_promise<T>> coroutine)
        const noexcept -> std::coroutine_handle<> {
      return coroutine.promise().calling_coroutine;
    };
  };

  auto initial_suspend() noexcept -> std::suspend_always { return {}; };
  auto final_suspend() noexcept -> final_awaitable { return {}; };
  auto unhandled_exception() -> void { std::terminate(); };

  auto set_calling_coroutine(std::coroutine_handle<> coroutine) -> void {
    calling_coroutine = coroutine;
  }

public:
  std::coroutine_handle<> calling_coroutine = std::noop_coroutine();
};

template <typename T> class task_promise final : public task_promise_base<T> {
public:
  auto get_return_object() noexcept -> task<T> {
    return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
  };

  auto return_value(T &&value) noexcept -> void {
    value = std::forward<T>(value);
  };

  constexpr auto get_value() const noexcept -> T { return value; }

private:
  T value;
};

template <> class task_promise<void> final : public task_promise_base<void> {
public:
  auto get_return_object() noexcept -> task<void> {
    return task<void>{std::coroutine_handle<task_promise>::from_promise(*this)};
  };
  auto return_void() noexcept -> void{};
};
} // namespace co_uring_http

#endif
