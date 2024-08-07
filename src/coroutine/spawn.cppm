export module co_uring_http.coroutine:spawn;

import std;

import :trait;

namespace co_uring_http {

class spawn_task_promise;

class spawn_task {
public:
  using promise_type = spawn_task_promise;
};

class spawn_task_promise {
public:
  [[nodiscard]] auto get_return_object() const noexcept -> spawn_task {
    return spawn_task{};
  }

  [[nodiscard]] auto initial_suspend() const noexcept -> std::suspend_never {
    return std::suspend_never{};
  }

  [[nodiscard]] auto final_suspend() const noexcept -> std::suspend_never {
    return std::suspend_never{};
  }

  [[noreturn]] auto unhandled_exception() const noexcept -> void {
    std::terminate();
  }

  auto return_void() const noexcept -> void {}
};

export template <typename Awaitable>
  requires awaitable<Awaitable>
auto spawn(Awaitable &&awaitable) -> void {
  [&awaitable]() noexcept -> spawn_task {
    co_await std::forward<Awaitable>(awaitable);
  }();
}
}; // namespace co_uring_http
