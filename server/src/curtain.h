#ifndef CURTAIN_H_
#define CURTAIN_H_

#include <esp_err.h>
#include <expected>

namespace Curtain {

enum State {
    OPEN,
    CLOSED,
    UNKNOWN
};

static State state = UNKNOWN;

struct Config {
    uint8_t gpio_num;
    int group = 0;
    uint8_t endstops[2];
};

std::expected<void, esp_err_t> init(const Config& config);

// std::expected<void, esp_err_t> set(State state);

}

#endif