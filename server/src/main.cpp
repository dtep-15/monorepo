#include <esp_littlefs.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <mdns.h>
#include <ArduinoJson.hpp>
#include <cstdio>
#include <format>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <nvs_flash.h>

#include <vector>

#include "esp_netif_sntp.h"
#include "esp_netif_types.h"
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

extern "C" void app_main() {
  if (esp_vfs_littlefs_register(&LittleFSConfig) != ESP_OK) {
    std::cout << "Filesystem failed to mount" << std::endl;
  }

  config = new Config(concat_paths(LittleFSConfig.base_path, "config.json"));
  esp_event_loop_create_default();
  esp_netif_init();
  auto ap_netif = esp_netif_create_default_wifi_ap();
  auto sta_netif = esp_netif_create_default_wifi_sta();

  esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&sntp_config);

  nvs_flash_init();

  WiFi::init();
  if (!(config->ssid.has_value() && WiFi::STA::connect(config->ssid.value(), config->password).has_value())) {
    WiFi::AP::start("Curtain Opener");
  }

  // clang-format off
  Server::init(LittleFSConfig.base_path, {
  {"state", HTTP_GET, [](const auto req) {
    Json::JsonDocument doc; 
    wifi_ap_record_t record;
    doc["is_configured"] = config->ssid.has_value() && esp_wifi_sta_get_ap_info(&record) == ESP_OK;
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
        object["password_required"] = network.authmode != WIFI_AUTH_OPEN;
      }
      std::string result;
      Json::serializeJson(doc, result);
      return Server::API::Response(result, "200 OK");
    }},
  {"networks/connect", HTTP_POST, [](const auto req) {
    Json::JsonDocument doc;
    Json::DeserializationError err;
    if ((err = Json::deserializeJson(doc, req.content))) {
      std::cout << err << std::endl;
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
      doc["open_at"] = config->open_time;
      doc["close_at"] = config->close_time;
      std::string result;
      Json::serializeJson(doc, result);
      return Server::API::Response(result, "200 OK");
    }}, {"schedule", HTTP_POST, [](const auto req) {
      Json::JsonDocument doc;
      Json::deserializeJson(doc, req.content);
      config->open_time = doc["open_at"] | 60*6;
      config->close_time = doc["close_at"] | 60*18;
      config->save();
      return Server::API::Response("", "200 OK");
    }}}
  );
  // clang-format on 
  mdns_init();
  mdns_hostname_set("curtain");
  Curtain::init({.gpio_num = 12});
}