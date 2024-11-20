#include "curtain.h"

#include <driver/gpio.h>
#include <driver/mcpwm_prelude.h>
#include <freertos/FreeRTOS.h>
#include <iostream>

#include "common.h"

namespace Curtain {

std::expected<void, esp_err_t> init(const Config& config) {
  esp_err_t result = ESP_OK;

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

  mcpwm_cmpr_handle_t comparator = nullptr;
  mcpwm_comparator_config_t comparator_config = {
      .flags = {.update_cmp_on_tez = true},
  };
  RIE(mcpwm_new_comparator(oper, &comparator_config, &comparator))

  mcpwm_gen_handle_t generator = nullptr;
  mcpwm_generator_config_t generator_config = {.gen_gpio_num = config.gpio_num,
                                               .flags = {.pull_up = true, .pull_down = false}};

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
      RIE(mcpwm_comparator_set_compare_value(comparator, 2'000));
      vTaskDelay(pdMS_TO_TICKS(2000));
      std::cout << "Off" << std::endl;
      RIE(mcpwm_comparator_set_compare_value(comparator, 1'500));
      vTaskDelay(pdMS_TO_TICKS(2000));
      std::cout << "Reverse" << std::endl;
      RIE(mcpwm_comparator_set_compare_value(comparator, 1'000));
      vTaskDelay(pdMS_TO_TICKS(2000));
    } */

  return {};
}

// std::expected<void, esp_err_t> set(State state) {}

}  // namespace Curtain