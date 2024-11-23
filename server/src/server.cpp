#include "server.h"

#include <esp_http_server.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common.h"

namespace Server {

const std::unordered_map<std::string, const char*> mimes = {{"html", "text/html"},
                                                            {"css", "text/css"},
                                                            {"js", "text/javascript"},
                                                            {"wasm", "application/wasm"}};

static httpd_config_t SERVER_CONFIG = HTTPD_DEFAULT_CONFIG();
static httpd_handle_t SERVER_HANDLE = NULL;

static std::string FS_BASE;
static std::vector<API> APIS;

static esp_err_t file_handler(httpd_req_t* req, std::string canonicalized) {
  if (req->method != HTTP_GET) {
    return ESP_FAIL;
  }
  static const std::string public_prefix = concat_paths(FS_BASE, "public");

  if (canonicalized == "") {
    canonicalized = "index.html";
    std::cout << "Serving index" << std::endl;
  }

  std::string final_path = concat_paths(public_prefix, canonicalized);

  std::ifstream file(final_path, std::ios::binary);
  if (file.fail()) {
    return httpd_resp_send_404(req);
  }

  std::string extension(final_path.begin() + final_path.rfind(".") + 1, final_path.end());
  if (mimes.contains(extension)) {
    // const char* mime = "text/html";
    // if (extension == "html") {
    //   mime = "text/html";
    // } else if (extension == "css") {
    //   mime = "text/css";
    // } else if (extension == "js") {
    //   mime = "text/javascript";
    // } else if (extension == "wasm") {
    //   mime = "application/wasm";
    // }
    httpd_resp_set_type(req, mimes.at(extension));
  }

  while (true) {
    std::array<char, 512> buf;
    file.read(buf.data(), buf.size());
    /*std::cout << esp_err_to_name( */ httpd_resp_send_chunk(req, buf.data(), file.gcount()) /* ) << std::endl*/;
    if (file.eof()) {
      break;
    }
  }
  std::cout << "Sending last chunk" << std::endl;
  std::cout << esp_err_to_name(httpd_resp_send_chunk(req, "", 0)) << std::endl;
  return ESP_OK;

  /*   if (file.has_value()) {
      std::string extension(final_path.begin() + final_path.rfind(".") + 1, final_path.end());
      if (mimes.contains(extension)) {
        std::string mime = mimes.at(extension);
        httpd_resp_set_type(req, mime.c_str());
      }
      return httpd_resp_send(req, file.value().data(), file.value().size());
    } else {
      return httpd_resp_send_404(req);
    } */
}

static esp_err_t api_handler(httpd_req_t* req, std::string canonicalized) {
  for (const auto& api : APIS) {
    if ((api.method == HTTP_ANY || api.method == req->method) &&
        std::strncmp(&canonicalized[5], api.name.c_str(), canonicalized.size() - 5) == 0) {
      std::string content(req->content_len, '\0');
      httpd_req_recv(req, content.data(), content.size());
      const auto response = api.callback({req->method, content});
      httpd_resp_set_status(req, response.status.c_str());
      return httpd_resp_send(req, response.content.c_str(), response.content.size());
    }
  }
  return httpd_resp_send_404(req);
}

static esp_err_t main_handler(httpd_req_t* req) {
  auto canonicalized = canonicalize_file(std::string(req->uri));
  if (canonicalized.has_value()) {
    if (std::strncmp(canonicalized.value().c_str(), "/api/", 5) == 0) {
      return api_handler(req, canonicalized.value());
    } else {
      return file_handler(req, canonicalized.value());
    }
  } else {
    return httpd_resp_send_404(req);
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
  SERVER_CONFIG.stack_size = 16384;
  SERVER_CONFIG.uri_match_fn = httpd_uri_match_wildcard;
  SERVER_CONFIG.max_uri_handlers = 1;
  SERVER_CONFIG.max_resp_headers = 16;
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