#include "sync_wait.hpp"

auto co_uring_http::sync_wait_task::wait() const noexcept -> void {
  coroutine.promise().wait();
}
