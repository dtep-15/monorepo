#ifndef SERVER_H_
#define SERVER_H_

#include <esp_err.h>
#include <expected>
#include <string>

namespace Server {

std::expected<void, esp_err_t> init(const std::string fs_base);
std::expected<void, esp_err_t> stop();

}

#endif