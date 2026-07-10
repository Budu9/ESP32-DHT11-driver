#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPIO_PIN GPIO_NUM_4
#define BIT_MASK                                                               \
  (1ULL) << GPIO_PIN // ...0001000 for bit position 4, 1ULL for 64 bit number

static int wait_for_level_change(int expected_level, int timeout) {
  int counter = 0;
  while (gpio_get_level(GPIO_PIN) == expected_level) {
    if (counter >= timeout) {
      return -1;
    }
    counter++;
    esp_rom_delay_us(1);
  }
  return counter;
}

void start_seq() {
  // set pin 4 as output
  gpio_set_direction(GPIO_PIN, GPIO_MODE_OUTPUT);

  // set level to high initially

  gpio_set_level(GPIO_PIN, 1);

  // output high for 1ms
  esp_rom_delay_us(1000);

  // not pull low for 18 ms as per docs

  gpio_set_level(GPIO_PIN, 0);
  esp_rom_delay_us(18000);

  // now pull up voltage for 20-40 microseconds
  gpio_set_level(GPIO_PIN, 1);
  esp_rom_delay_us(40);

  // now listen for response
  gpio_set_direction(GPIO_PIN, GPIO_MODE_INPUT);
  // gpio_set_pull_mode(GPIO_PIN, GPIO_PULLUP_ONLY);

  if (wait_for_level_change(0, 100) == -1) {
    printf("timeout\n");
  }

  if (wait_for_level_change(1, 100) == -1) {
    printf("timeout\n");
  }
  printf("startup seq worked\n");
}

void app_main(void) {

  // initialise config obj with 0s
  gpio_config_t io_conf = {};

  // disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;

  // set as output mode for starting sequence
  io_conf.mode = GPIO_MODE_OUTPUT;

  // bit mask of the pins that you want to set in this case 4 by default
  io_conf.pin_bit_mask = BIT_MASK;

  // disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  // disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  // configure GPIO with the given settings
  gpio_config(&io_conf);

  // initialise high for start sequence
  gpio_set_level(GPIO_PIN, 1);

  vTaskDelay(pdMS_TO_TICKS(1000));

  start_seq();
}
