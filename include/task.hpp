#ifndef TASK_HPP
#define TASK_HPP

#include <coroutine>
#include <optional>

namespace co_uring_http {
template <typename T> class task_promise;

template <typename T = void> class [[nodiscard]] task {
public:
  using promise_type = task_promise<T>;

  explicit task(std::coroutine_handle<task_promise<T>> handle) noexcept
      : handle(handle){};

  ~task() noexcept {
    if (handle.has_value()) {
      handle.value().destroy();
    }
  };

  task(task &&other) noexcept : handle(other.handle) {
    other.handle = std::nullopt;
  };

  auto operator=(task &&other) noexcept -> task & {
    if (this == std::addressof(other)) {
      return *this;
    }

    if (handle.has_value()) {
      handle.value().destroy();
    }
    handle = std::exchange(other.handle, nullptr);
    return *this;
  };

  task(const task &other) = delete;

  auto operator=(const task &other) -> task & = delete;

private:
  std::optional<std::coroutine_handle<task_promise<T>>> handle;
};

template <typename T> class task_promise {
public:
  auto get_return_object() noexcept -> task<T> {
    return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
  };
  auto initial_suspend() noexcept -> std::suspend_never { return {}; };
  auto final_suspend() noexcept -> std::suspend_never { return {}; };
  auto return_value(T &&value) noexcept -> void { return; };
  auto unhandled_exception() -> void { return; };
};

template <> class task_promise<void> {
public:
  auto get_return_object() noexcept -> task<void> {
    return task{std::coroutine_handle<task_promise>::from_promise(*this)};
  };
  auto initial_suspend() noexcept -> std::suspend_never { return {}; };
  auto final_suspend() noexcept -> std::suspend_never { return {}; };
  auto return_void() noexcept -> void { return; };
  auto unhandled_exception() -> void { return; };
};
} // namespace co_uring_http

#endif
