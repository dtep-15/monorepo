#include "common.h"

#include <fstream>

std::expected<std::vector<char>, esp_err_t> read_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (file.fail()) {
    return std::unexpected(ESP_FAIL);
  }
  std::vector<char> buf(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(buf.data(), buf.size());
  return buf;
}

std::optional<std::string> canonicalize_file(const std::string_view& path) {
  auto view_begin = path.begin();
  auto view_end = path.end();
  std::vector<std::string_view> nodes;
  do {
    std::string_view current_view(view_begin, view_end);
    size_t next_slash = current_view.find("/");
    if (next_slash != std::string::npos) {
      view_end = view_begin + next_slash;
    }
    current_view = std::string_view(view_begin, view_end);
    if (current_view == "..") {
      if (nodes.empty()) {
        return {};
      }
      nodes.pop_back();
    } else if (!current_view.empty()) {
      nodes.push_back(current_view);
    }
    if (view_end == path.end()) {
      break;
    }
    view_begin = view_end + 1;
    view_end = path.end();
  } while (view_begin != view_end);
  std::string result;
  result.reserve(path.size());
  for (auto& node : nodes) {
    result += "/";
    result += node;
  }
  return result;
}

std::string concat_paths(std::string_view first, std::string_view second) {
  if (first.ends_with("/")) {
    first = std::string_view(first.begin(), first.end() - 1);
  }
  if (second.starts_with("/")) {
    second = std::string_view(second.begin() + 1, second.end());
  }
  std::string result;
  result.reserve(first.length() + second.length() + 1);
  result += first;
  result += "/";
  result += second;
  return result;
}