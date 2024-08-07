export module co_uring_http.http:message;

import std;

namespace co_uring_http {

export class http_header {
public:
  std::string name;
  std::string value;
};

export class http_request {
public:
  std::string method;
  std::string url;
  std::string version;
  std::vector<http_header> headers;
};

export class http_response {
public:
  std::string version;
  std::string status;
  std::string status_text;
  std::vector<http_header> headers;
};

export class http_parser {
public:
  auto consume_packet(const std::span<std::uint_least8_t> packet) noexcept
      -> std::optional<http_request> {
    packet_list_.emplace_back(packet);
    if (!std::ranges::ends_with(packet, std::string("\r\n\r\n"))) {
      return std::nullopt;
    }

    auto request_lines = packet_list_ | std::views::join |
                         std::views::split(std::string_view("\r\n")) |
                         std::ranges::to<std::vector<std::string>>();
    packet_list_.clear();

    auto status_line = request_lines[0] | std::views::split(' ') |
                       std::ranges::to<std::vector<std::string>>();

    http_request http_request;
    http_request.method = status_line[0];
    http_request.url = status_line[1];
    http_request.version = status_line[2];
    http_request.headers =
        request_lines | std::views::drop(1) |
        std::views::filter(
            [](const std::string_view header_line) static noexcept -> bool {
              return header_line.size() != 0;
            }) |
        std::views::transform(std::views::split(':') |
                              std::ranges::to<std::vector<std::string>>()) |
        std::views::transform(
            [](const std::vector<std::string> &header_line) static noexcept
            -> http_header {
              return http_header{header_line[0], header_line[1]};
            }) |
        std::ranges::to<std::vector<http_header>>();
    return http_request;
  }

private:
  std::vector<std::span<std::uint_least8_t>> packet_list_;
};

} // namespace co_uring_http

template <> class std::formatter<co_uring_http::http_response> {
public:
  template <class ParseContext>
  [[nodiscard]] constexpr auto parse(ParseContext &context) const {
    return context.begin();
  }

  template <typename FormatContext>
  [[nodiscard]] auto
  format(const co_uring_http::http_response &response,
         FormatContext &context) const -> FormatContext::iterator {
    return std::format_to(
        context.out(), "{} {} {}\r\n{:s}\r\n", response.version,
        response.status, response.status_text,
        response.headers |
            std::views::transform([](const auto &header) static noexcept {
              return std::format("{}: {}\r\n", header.name, header.value);
            }) |
            std::views::join);
  }
};
