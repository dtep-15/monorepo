#ifndef SERVER_H_
#define SERVER_H_

#include <esp_err.h>
#include <esp_http_server.h>
#include <expected>
#include <string>
#include <vector>

namespace Server {

struct API {
  struct Request{
    int method;
    const std::string& content;
  };
  struct Response {
    std::string content = "";
    std::string status = "200 OK";
  };
  std::string name;
  http_method method;
  Response (*callback)(const Request);
};

std::expected<void, esp_err_t> init(const std::string fs_base, const std::vector<API> apis);
std::expected<void, esp_err_t> stop();

}  // namespace Server

#endif