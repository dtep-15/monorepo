#ifndef CONFIG_H_
#define CONFIG_H_

#include <esp_err.h>
#include <optional>
#include <string>

class Config {
 public:
  Config(std::string location);
  uint32_t open_time = 21600; // 6 AM
  uint32_t close_time = 64800; // 6 PM
  std::optional<std::string> ssid;
  std::optional<std::string> password;
  esp_err_t save();

 private:
  const std::string location_;
};

#endif