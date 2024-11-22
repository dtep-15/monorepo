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
static std::vector<API> APIS;

static esp_err_t file_handler(httpd_req_t* req) {
  if (req->method != HTTP_GET) {
    return ESP_FAIL;
  }
  static const std::string public_prefix = concat_paths(FS_BASE, "public");

  auto canonical = canonicalize_file(req->uri);
  if (!canonical.has_value()) {
    return httpd_resp_send_404(req);
  }

  if (canonical.value() == "") {
    canonical = "index.html";
    std::cout << "Serving index" << std::endl;
  }

  std::string final_path = concat_paths(public_prefix, canonical.value());

  const auto file = read_file(final_path);

  if (file.has_value()) {
    return httpd_resp_send(req, file.value().data(), file.value().size());
  } else {
    return httpd_resp_send_404(req);
  }
}

static esp_err_t api_handler(httpd_req_t* req) {
  for (const auto& api : APIS) {
    if ((api.method == HTTP_ANY || api.method == req->method) &&
        std::strncmp(&req->uri[5], api.name.c_str(), sizeof(req->uri) - 5) == 0) {
      std::string content;
      content.reserve(req->content_len);
      httpd_req_recv(req, content.data(), content.size());
      const auto response = api.callback({req->method, content});
      httpd_resp_set_status(req, response.status.c_str());
      return httpd_resp_send(req, response.content.c_str(), response.content.size());
    }
  }
  return httpd_resp_send_404(req);
}

static esp_err_t main_handler(httpd_req_t* req) {
  if (std::strncmp(req->uri, "/api/", 5) == 0) {
    return api_handler(req);
  } else {
    return file_handler(req);
  }
}

static const httpd_uri_t all_uris = {
    .uri = "*",
    .method = (httpd_method_t)HTTP_ANY,
    .handler = main_handler,
    .user_ctx = nullptr,
};

std::expected<void, esp_err_t> init(const std::string fs_base, const std::vector<API> apis) {
  FS_BASE = std::move(fs_base);
  APIS = std::move(apis);
  SERVER_CONFIG.uri_match_fn = httpd_uri_match_wildcard;
  SERVER_CONFIG.max_uri_handlers = 1;
  esp_err_t result = ESP_OK;
  RIE(httpd_start(&SERVER_HANDLE, &SERVER_CONFIG))
  RIE(httpd_register_uri_handler(SERVER_HANDLE, &all_uris))
  return {};
}

std::expected<void, esp_err_t> stop() {
  esp_err_t result = ESP_OK;
  RIE(httpd_stop(SERVER_HANDLE))
  return {};
}

}  // namespace Server