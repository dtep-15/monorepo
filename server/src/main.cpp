#include <esp_littlefs.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <mdns.h>
#include <ArduinoJson.hpp>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>
#include <format>
#include <cstdio>

#include "esp_vfs.h"

#include "common.h"
#include "config.h"
#include "curtain.h"
#include "server.h"
#include "wifi.h"

namespace Json = ArduinoJson;

const static esp_vfs_littlefs_conf_t LittleFSConfig{
    .base_path = "/littlefs",
    .partition_label = "littlefs",
    .format_if_mount_failed = false,
    .read_only = false,
    .dont_mount = false,
    .grow_on_mount = false,
};

Config* config;

std::string format_time(uint32_t time) {
  uint8_t hours = time / (60 * 60);
  uint8_t minutes = (time / 60) - (hours * 60);
  uint8_t seconds = time % 60;
  return std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
}

uint32_t parse_time(const std::string_view& time) {
  uint8_t seconds, minutes, hours;
  std::sscanf(time.data(), "%hhu:%hhu:%hhu", &hours, &minutes, &seconds);
  return ((hours * 60) + minutes) * 60 + seconds;
}

extern "C" void app_main() {
  esp_vfs_littlefs_register(&LittleFSConfig);

  config = new Config(concat_paths(LittleFSConfig.base_path, "config.json"));
  esp_event_loop_create_default();
  esp_netif_init();
  auto ap_netif = esp_netif_create_default_wifi_ap();
  auto sta_netif = esp_netif_create_default_wifi_sta();

  WiFi::init();
  if (config->ssid.has_value()) {
    WiFi::STA::connect(config->ssid.value(), config->password);
  } else {
    WiFi::AP::start("Uninitialized Curtain Opener");
  }
  // clang-format off
  Server::init(LittleFSConfig.base_path, {
  {"state", HTTP_GET, [](const auto req) {
    Json::JsonDocument doc; 
    if (config->ssid.has_value()) {
        doc["is_configured"] = true;
        doc["ssid"] = config->ssid.value();
    } else {
        doc["is_configured"] = false;
        doc["ssid"] = nullptr;
    }
    std::string result;
    Json::serializeJson(doc, result);
    return Server::API::Response(result, "200 OK");
  }},
  {"networks", HTTP_GET,
    [](const auto req) {
      Json::JsonDocument doc;
      const auto networks = WiFi::STA::scan();
      if (!networks.has_value()) {
        return Server::API::Response(esp_err_to_name(networks.error()),
                                    "500 Internal Server Error");
      }
      for (const auto& network : networks.value()) {
        auto object = doc.add<Json::JsonObject>();
        object["name"] = network.ssid;
        object["password_protected"] = network.authmode != WIFI_AUTH_OPEN;
      }
      std::string result;
      Json::serializeJson(doc, result);
      return Server::API::Response(result, "200 OK");
    }},
  {"networks/connect", HTTP_POST, [](const auto req) {
    Json::JsonDocument doc;
    if (Json::deserializeJson(doc, req.content)) {
      return Server::API::Response("", "400 Bad Request");
    }
    std::string ssid = doc["name"] | "";
    std::string password = doc["password"] | "";
    if (ssid.empty()) {
      return Server::API::Response("SSID is required", "400 Bad Request");
    }
    bool connected = true; 
    // TODO: connect method to return status
    WiFi::STA::connect(ssid, password);
    if (!connected) {
      return Server::API::Response("Failed to connect", "403 Forbidden");
    }
    config->ssid = ssid;
    config->password = password;
    config->save();
    return Server::API::Response("", "200 OK");
    }},
  {"schedule", HTTP_GET, [](const auto req) {
      Json::JsonDocument doc;
      doc["open_at"] = format_time(config->open_time);
      doc["close_at"] = format_time(config->close_time);
      std::string result;
      Json::serializeJson(doc, result);
      return Server::API::Response(result, "200 OK");
    }}, {"schedule", HTTP_POST, [](const auto req) {
      Json::JsonDocument doc;
      Json::deserializeJson(doc, req.content);
      config->open_time = parse_time(doc["open_at"] | "6:0:0");
      config->close_time = parse_time(doc["close_at"] | "18:0:0");
      config->save();
      return Server::API::Response("", "200 OK");
    }}}
  );
  // clang-format on 
  mdns_init();
  mdns_hostname_set("curtain");
  // Curtain::init({.gpio_num = 14, .resolution=1'000'000, .period_ticks=20'000});
}