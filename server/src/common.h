#ifndef COMMON_H_
#define COMMON_H_

#include <esp_err.h>
#include <expected>

static const char* TAG = __FILE__;

#define RIE(f)                      \
  if ((result = f) != ESP_OK) {     \
    return std::unexpected(result); \
  }

#endif