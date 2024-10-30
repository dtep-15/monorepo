#include <esp_littlefs.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <mdns.h>
#include <iostream>
#include <string>

#include "common.h"
#include "server.h"
#include "wifi.h"

const static esp_vfs_littlefs_conf_t LittleFSConfig{
    .base_path = "/littlefs",
    .partition_label = "littlefs",
    .partition = nullptr,
    .format_if_mount_failed = false,
    .read_only = false,
    .dont_mount = false,
    .grow_on_mount = false,
};

extern "C" void app_main() {
  // esp_vfs_littlefs_register(&LittleFSConfig);
  esp_event_loop_create_default();
  esp_netif_init();
  auto ap_netif = esp_netif_create_default_wifi_ap();
  auto sta_netif = esp_netif_create_default_wifi_sta();
  WiFi::init();
  WiFi::AP::start("Uninitialized Curtain Opener");
  Server::init(LittleFSConfig.base_path);
  mdns_init();
  mdns_hostname_set("curtain");
  // esp_vfs_littlefs_unregister(LittleFSConfig.partition_label);
}