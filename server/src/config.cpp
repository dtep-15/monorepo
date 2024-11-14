#include "config.h"

#include "common.h"

#include <ArduinoJson.hpp>
#include <fstream>

namespace Json = ArduinoJson;

Config::Config(const std::string location) : location_(std::move(location)) {
  const auto file = read_file(location_);
  if (file.has_value()) {
    Json::JsonDocument doc;
    Json::deserializeJson(doc, file.value());
    open_time = doc["open_time"] | open_time;
    close_time = doc["close_time"] | close_time;
    if (!doc["ssid"].isNull()) {
      ssid = std::optional<std::string>(doc["ssid"]);
    }
    if (!doc["password"].isNull()) {
      password = std::optional<std::string>(doc["password"]);
    }
  }
}

esp_err_t Config::save() {
  Json::JsonDocument doc;
  doc["open_time"] = open_time;
  doc["close_time"] = close_time;
  if (ssid.has_value()) {
    doc["ssid"] = ssid.value();
  }
  if (password.has_value()) {
    doc["password"] = password.value();
  }
  std::string result;
  Json::serializeJson(doc, result);
  std::ofstream file(location_, std::ios::binary | std::ios::trunc);
  if (file.fail()) {
    return ESP_FAIL;
  }
  file.write(result.c_str(), result.size());
  if (file.fail()) {
    return ESP_FAIL;
  }
  return ESP_OK;
}