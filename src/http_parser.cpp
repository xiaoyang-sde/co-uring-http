#include "http_parser.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <string_view>
#include <tuple>
#include <vector>

#include "http_message.hpp"

namespace co_uring_http {

auto trim_whitespace(std::string_view string) -> std::string_view {
  const auto first = std::find_if_not(string.cbegin(), string.cend(), [](unsigned char c) {
    return std::isspace(c);
  });
  const auto last = std::find_if_not(string.crbegin(), string.crend(), [](unsigned char c) {
                      return std::isspace(c);
                    }).base();
  return (last <= first) ? std::string_view()
                         : std::string_view(&*first, static_cast<std::size_t>(last - first));
}

auto split(std::string_view string, const char delimiter) -> std::vector<std::string_view> {
  std::vector<std::string_view> result;
  size_t segment_start = 0;
  size_t segment_end = 0;

  while ((segment_end = string.find(delimiter, segment_start)) != std::string::npos) {
    std::string_view token = string.substr(segment_start, segment_end - segment_start);
    result.emplace_back(token);
    segment_start = segment_end + 1;
  }

  result.emplace_back(string.substr(segment_start));
  return result;
}

auto split(std::string_view string, std::string_view delimiter) -> std::vector<std::string_view> {
  std::vector<std::string_view> result;
  size_t segment_start = 0;
  size_t segment_end = 0;
  const size_t delimiter_length = delimiter.length();

  while ((segment_end = string.find(delimiter, segment_start)) != std::string::npos) {
    std::string_view token = string.substr(segment_start, segment_end - segment_start);
    result.emplace_back(token);
    segment_start = segment_end + delimiter_length;
  }

  result.emplace_back(string.substr(segment_start));
  return result;
}

auto http_parser::parse_packet(std::span<std::byte> packet) -> std::optional<http_request> {
  raw_http_request_.reserve(raw_http_request_.size() + packet.size());
  std::transform(
      packet.begin(), packet.end(), std::back_inserter(raw_http_request_),
      [](const std::byte byte) -> unsigned char { return static_cast<unsigned char>(byte); }
  );

  std::string_view packet_string_view(reinterpret_cast<char *>(packet.data()));
  if (!packet_string_view.ends_with("\r\n\r\n")) {
    return {};
  }

  http_request http_request;
  std::string_view raw_http_request(reinterpret_cast<char *>(raw_http_request_.data()));
  const std::vector<std::string_view> request_line_list = split(raw_http_request, "\r\n");
  const std::vector<std::string_view> status_line_list = split(request_line_list[0], ' ');
  http_request.method = status_line_list[0];
  http_request.url = status_line_list[1];
  http_request.version = status_line_list[2];

  for (size_t line_index = 1; line_index < request_line_list.size(); ++line_index) {
    const std::string_view header_line = request_line_list[line_index];
    const std::vector<std::string_view> header = split(header_line, ':');
    if (header.size() == 2) {
      http_request.header_list.emplace_back(header[0], trim_whitespace(header[1]));
    }
  }

  raw_http_request_.clear();
  return http_request;
}
} // namespace co_uring_http
