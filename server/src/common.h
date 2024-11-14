#ifndef COMMON_H_
#define COMMON_H_

#include <esp_err.h>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

static const char* TAG = __FILE__;

#define RIE(f)                      \
  if ((result = f) != ESP_OK) {     \
    return std::unexpected(result); \
  }

std::expected<std::vector<char>, esp_err_t> read_file(const std::string& path);

std::optional<std::string> canonicalize_file(const std::string_view& path);

std::string concat_paths(std::string_view first, std::string_view second);

#endif