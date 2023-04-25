#include "http_server.hpp"

auto main() -> int {
  co_uring_http::http_server http_server;
  http_server.listen("8080");
}
