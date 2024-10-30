#include "server.h"

#include <esp_http_server.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "common.h"

namespace Server {

static httpd_config_t SERVER_CONFIG = HTTPD_DEFAULT_CONFIG();
static httpd_handle_t SERVER_HANDLE = NULL;

static std::string FS_BASE;

static std::optional<std::string> canonicalize_file(const std::string_view& path) {
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

static std::string concat_paths(std::string_view first, std::string_view second) {
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

static esp_err_t api_handler(httpd_req_t* req) {
  return ESP_OK;
}

static esp_err_t file_handler(httpd_req_t* req) {
  if (req->method != HTTP_GET) {
    return ESP_FAIL;
  }
  static const std::string public_prefix = concat_paths(FS_BASE, "public");

  auto canonical = canonicalize_file(req->uri);
  if (!canonical.has_value()) {
    std::cout << "404" << std::endl;
    return httpd_resp_send_404(req);
  }

  std::string final_path = concat_paths(public_prefix, canonical.value());

  // std::ifstream file(, std::ios::binary | std::ios::ate);
  // std::vector<char> buf(file.tellg());
  // file.seekg(0, std::ios::beg);
  // file.read(buf.data(), buf.size());
  std::cout << final_path << std::endl;
  return httpd_resp_send(req, final_path.data(), final_path.size());
  return ESP_OK;
}

static esp_err_t main_handler(httpd_req_t* req) {
  if (std::strncmp(req->uri, "/api/", 5) == 0) {
    return api_handler(req);
  } else {
    return file_handler(req);
  }
}

static const httpd_uri_t all_uris = {
    .uri = "/*?",
    .method = HTTP_POST,
    .handler = nullptr,
    .user_ctx = nullptr,
};

std::expected<void, esp_err_t> init(const std::string fs_base) {
  FS_BASE = fs_base;
  SERVER_CONFIG.uri_match_fn = httpd_uri_match_wildcard;
  esp_err_t result = ESP_OK;
  RIE(httpd_start(&SERVER_HANDLE, &SERVER_CONFIG))
  RIE(httpd_register_uri_handler(&SERVER_HANDLE, &all_uris))
  return {};
}

std::expected<void, esp_err_t> stop() {
  esp_err_t result = ESP_OK;
  RIE(httpd_stop(&SERVER_HANDLE))
  return {};
}

}  // namespace Server