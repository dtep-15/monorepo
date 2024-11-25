#include "curtain.h"

#include <driver/gpio.h>
#include <driver/mcpwm_prelude.h>
#include <freertos/FreeRTOS.h>
#include <atomic>
#include <iostream>

#include "common.h"
#include "config.h"

extern Config* config;

namespace Curtain {

enum class State {
  Open,
  Closed,
  Unknown,
};

static volatile State current_state = State::Unknown;
static volatile State previous_state = State::Unknown;
volatile bool manual_toggle = false;

const gpio_num_t GPIO_OPEN_BTN = GPIO_NUM_13, GPIO_CLOSED_BTN = GPIO_NUM_14;

static void gpio_interrupt_handler(void* args);

static TaskHandle_t schedule_task_handle;
static mcpwm_cmpr_handle_t comparator = nullptr;

static std::expected<void, esp_err_t> move_to_state(State state) {
  esp_err_t result = ESP_OK;
  assert(state != State::Unknown);

  if (previous_state != current_state) {
    manual_toggle = false;
  }
  previous_state = current_state;

  if (manual_toggle) {
    if (state == State::Open) {
      state = State::Closed;
    } else {
      state = State::Open;
    }
  }

  // Pull-up pins
  bool open_btn_state = !gpio_get_level(GPIO_OPEN_BTN);
  bool closed_btn_state = !gpio_get_level(GPIO_CLOSED_BTN);

  // Open & closed buttons may already be pressed, in which case the interrupt won't get triggered
  if (open_btn_state) {
    current_state = State::Open;
  } else if (closed_btn_state) {
    current_state = State::Closed;
  }

  if (current_state == state) {
    std::cout << "Already set" << std::endl;
    return {};
  }

  if (state == State::Open) {
    std::cout << "Open" << std::endl;
    // Normal
    RIE(mcpwm_comparator_set_compare_value(comparator, 2'000));
  } else {
    std::cout << "Closed" << std::endl;
    // Reverse
    RIE(mcpwm_comparator_set_compare_value(comparator, 1'000));
  }

  return {};
}

void schedule_task(void* arg) {
  // function to get time: gettimeofday
  while (true) {
    struct timeval now;
    gettimeofday(&now, NULL);
    uint32_t now_minutes = ((now.tv_sec / 60) + 2 * 60) % (60 * 24);
    std::cout << now_minutes << std::endl;
    uint32_t open_time = config->open_time;
    uint32_t close_time = config->close_time;
    uint32_t earlier_time = std::min(open_time, close_time);
    uint32_t later_time = std::max(open_time, close_time);

    if (now_minutes >= earlier_time) {
      if (now_minutes >= later_time) {
        if (later_time == open_time) {
          move_to_state(State::Open);
        } else {
          move_to_state(State::Closed);
        }
      } else {
        if (earlier_time == open_time) {
          move_to_state(State::Open);
        } else {
          move_to_state(State::Closed);
        }
      }
    } else if (later_time == open_time) {
      move_to_state(State::Open);
    } else {
      move_to_state(State::Closed);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

std::expected<void, esp_err_t> init(const Config& config) {
  esp_err_t result = ESP_OK;

  gpio_install_isr_service(0);

  gpio_reset_pin(GPIO_OPEN_BTN);
  gpio_set_direction(GPIO_OPEN_BTN, GPIO_MODE_INPUT);
  gpio_pulldown_dis(GPIO_OPEN_BTN);
  gpio_pullup_en(GPIO_OPEN_BTN);
  gpio_set_intr_type(GPIO_OPEN_BTN, GPIO_INTR_NEGEDGE);
  gpio_isr_handler_add(GPIO_OPEN_BTN, gpio_interrupt_handler, (void*)GPIO_OPEN_BTN);

  gpio_reset_pin(GPIO_CLOSED_BTN);
  gpio_set_direction(GPIO_CLOSED_BTN, GPIO_MODE_INPUT);
  gpio_pulldown_dis(GPIO_CLOSED_BTN);
  gpio_pullup_en(GPIO_CLOSED_BTN);
  gpio_set_intr_type(GPIO_CLOSED_BTN, GPIO_INTR_NEGEDGE);
  gpio_isr_handler_add(GPIO_CLOSED_BTN, gpio_interrupt_handler, (void*)GPIO_CLOSED_BTN);

  mcpwm_timer_handle_t timer = nullptr;
  mcpwm_timer_config_t timer_config = {
      .group_id = config.group,
      .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
      .resolution_hz = 1'000'000,
      .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
      .period_ticks = 20'000,
  };
  RIE(mcpwm_new_timer(&timer_config, &timer))
  mcpwm_oper_handle_t oper = nullptr;
  mcpwm_operator_config_t operator_config = {.group_id = config.group};
  RIE(mcpwm_new_operator(&operator_config, &oper))
  mcpwm_operator_connect_timer(oper, timer);

  mcpwm_comparator_config_t comparator_config = {
      .flags = {.update_cmp_on_tez = true},
  };
  RIE(mcpwm_new_comparator(oper, &comparator_config, &comparator))

  mcpwm_gen_handle_t generator = nullptr;
  mcpwm_generator_config_t generator_config = {
      .gen_gpio_num = config.gpio_num,
      .flags = {.pull_up = true, .pull_down = false},
  };

  RIE(mcpwm_new_generator(oper, &generator_config, &generator));

  RIE(mcpwm_comparator_set_compare_value(comparator, 1'500));

  // go high on counter empty
  RIE(mcpwm_generator_set_action_on_timer_event(
      generator,
      MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
  // go low on compare threshold
  RIE(mcpwm_generator_set_action_on_compare_event(
      generator, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

  RIE(mcpwm_timer_enable(timer))
  RIE(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP))
  /*   while (true) {
      std::cout << "Full speed" << std::endl;
      std::cout << "Off" << std::endl;
      RIE(mcpwm_comparator_set_compare_value(comparator, 1'500));
      vTaskDelay(pdMS_TO_TICKS(2000));
      std::cout << "Reverse" << std::endl;
      RIE(mcpwm_comparator_set_compare_value(comparator, 1'000));
      vTaskDelay(pdMS_TO_TICKS(2000));
    } */

  auto task_result = xTaskCreate(schedule_task, "SCHEDULE_TASK", CONFIG_ESP_MAIN_TASK_STACK_SIZE, nullptr,
                                 tskIDLE_PRIORITY, &schedule_task_handle);
  if (task_result != pdPASS) {
    return std::unexpected(ESP_FAIL);
  }
  return {};
}

static void disable_servo() {
  mcpwm_comparator_set_compare_value(comparator, 1'500);
}

static void gpio_interrupt_handler(void* args) {
  int gpio_pin = (int)args;

  assert(gpio_pin == GPIO_OPEN_BTN || gpio_pin == GPIO_CLOSED_BTN);

  State next_state = gpio_pin == GPIO_OPEN_BTN ? State::Open : State::Closed;

  // Debouncing
  if (current_state == next_state)
    return;

  current_state = next_state;
  disable_servo();
}

}  // namespace Curtain