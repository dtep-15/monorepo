#include "wifi.h"

#include <nvs_flash.h>
#include <cstring>

#include "common.h"

namespace WiFi {

const static wifi_init_config_t INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
static wifi_mode_t MODE;

std::expected<void, esp_err_t> init() {
  esp_err_t result = ESP_OK;
  if (WIFI_NVS_ENABLED) {
    RIE(nvs_flash_init())
  }
  RIE(esp_wifi_init(&INIT_CONFIG))
  return {};
}

namespace AP {
static wifi_ap_config_t AP_CONFIG = {.max_connection = 1};

std::expected<void, esp_err_t> start(const std::string& ssid) {
  esp_err_t result = ESP_OK;
  switch (MODE) {
    case WIFI_MODE_APSTA:
    case WIFI_MODE_STA:
      MODE = WIFI_MODE_APSTA;
      break;
    default:
      MODE = WIFI_MODE_AP;
      break;
  }
  std::strncpy(reinterpret_cast<char*>(AP_CONFIG.ssid), ssid.c_str(), sizeof(AP_CONFIG.ssid));
  AP_CONFIG.ssid_len = ssid.length();
  RIE(esp_wifi_set_mode(MODE))
  RIE(esp_wifi_set_config(WIFI_IF_AP, reinterpret_cast<wifi_config_t*>(&AP_CONFIG)))
  RIE(esp_wifi_start())
  return {};
}
std::expected<void, esp_err_t> stop() {
  esp_err_t result = ESP_OK;
  switch (MODE) {
    case WIFI_MODE_APSTA:
    case WIFI_MODE_STA:
      MODE = WIFI_MODE_STA;
      RIE(esp_wifi_set_mode(MODE))
      RIE(esp_wifi_start())
      break;
    default:
      RIE(esp_wifi_stop());
      break;
  }
  return {};
}
}  // namespace AP

namespace STA {

static wifi_sta_config_t STA_CONFIG = {0};

std::expected<void, esp_err_t> connect(const std::string& ssid, const std::optional<std::string> password) {
  esp_err_t result = ESP_OK;
  switch (MODE) {
    case WIFI_MODE_APSTA:
    case WIFI_MODE_AP:
      MODE = WIFI_MODE_APSTA;
      break;
    default:
      MODE = WIFI_MODE_STA;
      break;
  }
  std::strncpy(reinterpret_cast<char*>(STA_CONFIG.ssid), ssid.c_str(), sizeof(STA_CONFIG.ssid));
  if (password.has_value()) {
    std::strncpy(reinterpret_cast<char*>(STA_CONFIG.password), password.value().c_str(), sizeof(STA_CONFIG.password));
    STA_CONFIG.sort_method = WIFI_CONNECT_AP_BY_SECURITY;
  }
  RIE(esp_wifi_set_mode(MODE))
  RIE(esp_wifi_set_config(WIFI_IF_STA, reinterpret_cast<wifi_config_t*>(&STA_CONFIG)))
  RIE(esp_wifi_start())
  return {};
}

std::expected<std::vector<wifi_ap_record_t>, esp_err_t> scan() {
  esp_err_t result = ESP_OK;
  switch (MODE) {
    case WIFI_MODE_APSTA:
    case WIFI_MODE_AP:
      MODE = WIFI_MODE_APSTA;
      break;
    default:
      MODE = WIFI_MODE_STA;
      break;
  }
  RIE(esp_wifi_set_mode(MODE))
  RIE(esp_wifi_start())
  uint16_t network_count;
  RIE(esp_wifi_scan_get_ap_num(&network_count))
  std::vector<wifi_ap_record_t> records(network_count, wifi_ap_record_t{});
  RIE(esp_wifi_scan_get_ap_records(&network_count, records.data()));
  return records;
}

std::expected<void, esp_err_t> stop() {
  esp_err_t result = ESP_OK;
  switch (MODE) {
    case WIFI_MODE_APSTA:
    case WIFI_MODE_AP:
      MODE = WIFI_MODE_AP;
      RIE(esp_wifi_set_mode(MODE))
      RIE(esp_wifi_start())
      break;
    default:
      RIE(esp_wifi_stop())
      break;
  }
  return {};
}
}  // namespace STA

}  // namespace WiFi