#ifndef WIFI_H_
#define WIFI_H_

#include <esp_err.h>
#include <esp_wifi.h>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace WiFi {

struct APConfig {
  std::string ssid;
  std::string password = "";
  wifi_auth_mode_t auth_mode = WIFI_AUTH_OPEN;
  bool hidden = false;
  uint8_t max_connections = 10;
};

std::expected<void, esp_err_t> init();

namespace AP {
std::expected<void, esp_err_t> start(const std::string& ssid);
/* std::expected<void, esp_err_t> change_ssid(const std::string& new_ssid);
std::expected<void, esp_err_t> change_auth(wifi_auth_mode_t mode, std::string password = "");
std::expected<void, esp_err_t> set_hidden(bool hidden);
std::expected<void, esp_err_t> set_max_connections(uint8_t connections); */
}  // namespace AP

namespace STA {
std::expected<void, esp_err_t> connect(const std::string& ssid, const std::optional<std::string> password);
std::expected<std::vector<wifi_ap_record_t>, esp_err_t> scan();
}  // namespace STA
}  // namespace WiFi

#endif