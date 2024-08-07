export module co_uring_http.coroutine:trait;

import std;

namespace co_uring_http {

template <typename T>
concept await_suspend_result = std::same_as<T, void> || std::same_as<T, bool> ||
                               std::same_as<T, std::coroutine_handle<>>;

export template <typename T>
concept awaiter = requires(T t) {
  { t.await_ready() } -> std::same_as<bool>;
  { t.await_suspend(nullptr) } -> await_suspend_result;
  { t.await_resume() };
};

export template <typename T>
concept awaitable = awaiter<T> || requires(T t) {
  { t.operator co_await() } -> awaiter;
} || requires(T t) {
  { operator co_await(t) } -> awaiter;
};

template <typename T>
  requires awaiter<T>
auto awaiter_t_impl() -> T;

template <typename T>
  requires awaitable<T>
auto awaiter_t_impl() -> decltype(std::declval<T>().operator co_await());

template <typename T>
  requires awaitable<T>
auto awaiter_t_impl() -> decltype(operator co_await(std::declval<T>()));

export template <typename T> using awaiter_t = decltype(awaiter_t_impl<T>());

export template <typename T>
  requires awaitable<T>
using await_result_t = decltype(std::declval<awaiter_t<T>>().await_resume());

}; // namespace co_uring_http
