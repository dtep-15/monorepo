#ifndef CONFIG_H_
#define CONFIG_H_

#include <esp_err.h>
#include <optional>
#include <string>

class Config {
 public:
  Config(std::string location);
  uint16_t open_time = 60*6; // 6 AM
  uint16_t close_time = 60*18; // 6 PM
  std::optional<std::string> ssid;
  std::optional<std::string> password;
  esp_err_t save();

 private:
  const std::string location_;
};

#endif