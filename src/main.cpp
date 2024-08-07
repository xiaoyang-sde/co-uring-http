import std;

import co_uring_http;

auto main() -> std::int_least32_t {
  co_uring_http::http_server http_server;
  http_server.listen();
  return 0;
}
