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
static int read_bit(void);
static int read_5_bytes(uint8_t buffer[5]);
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
void start_seq(uint8_t buffer[5]) {
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
  gpio_set_pull_mode(GPIO_PIN, GPIO_PULLUP_ONLY);

  portDISABLE_INTERRUPTS();
  // start seq low
  int r1 = wait_for_level_change(0, 100);
  // start seq high
  int r2 = wait_for_level_change(1, 100);
  int read_result = -1;
  if (r1 != -1 && r2 != -1) {
    read_result = read_5_bytes(buffer);
  }
  portENABLE_INTERRUPTS(); // RTOS level interrupts disabled

  if (r1 == -1 || r2 == -1) {
    printf("timeout\n");
    return;
  }
  printf("startup seq worked\n");
  if (read_result == 0) {
    printf("5 bytes read\n");
  } else {
    printf("failure to read 5 bytes\n");
    return;
  }
  uint8_t checksum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
  if (checksum == buffer[4]) {
    printf("temp: %d, humidity: %d\n", buffer[2], buffer[0]);
  } else {
    printf("expected checksum %d but got %d\n", buffer[4], checksum);
  }
}
static int read_bit(void) {
  if (wait_for_level_change(0, 60) == -1) {
    return -1;
  }
  int duration = wait_for_level_change(1, 150);
  if (duration == -1) {
    return -1;
  }
  return duration > 45 ? 1 : 0; // short high (28 microseconds) = 0, long high (
                                // 70 microseconds) = 1
}
static int read_5_bytes(uint8_t buffer[5]) {
  for (int i = 0; i < 40; i++) {
    int val = read_bit();
    if (val == -1) {
      return val;
    }
    buffer[i / 8] <<= 1; // shift every bit
    if (val == 1) {
      buffer[i / 8] |= 1; // OR in the new bit
    }
    // if 0 dont do anything
  }
  return 0;
}
void app_main(void) {
  uint8_t buffer[5] = {0, 0, 0, 0, 0};
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
  vTaskDelay(pdMS_TO_TICKS(1000)); // needed for boot up sequence of sensor
  start_seq(buffer);
}
