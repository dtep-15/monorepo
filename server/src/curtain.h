#ifndef CURTAIN_H_
#define CURTAIN_H_

#include <esp_err.h>
#include <expected>

namespace Curtain {

struct Config {
    uint8_t gpio_num;
    int group = 0;
};

std::expected<void, esp_err_t> init(const Config& config);

// std::expected<void, esp_err_t> set(State state);

}

#endif