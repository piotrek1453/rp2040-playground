#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "network_settings.h"
#include "pico/stdlib.h"
#include "port_common.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"
#include <stdio.h>

// SPI Defines for W5500
#define W5500_SPI_PORT spi0
#define W5500_PIN_MISO 16
#define W5500_PIN_CS 17
#define W5500_PIN_SCK 18
#define W5500_PIN_MOSI 19
// LED define
#define LED_PIN 25

// Network defines and vars
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)
#define HTTP_SOCKET_MAX_NUM 4
static uint8_t g_http_send_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_http_recv_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_http_socket_num_list[HTTP_SOCKET_MAX_NUM] = {0, 1, 2, 3};

// Timer for heartbeat interrupt
struct repeating_timer timer;

bool timer_heartbeat_callback(struct repeating_timer *t) {
  // toggle LED
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
  // print a message to the console
  printf("I'm alive!\r\n");
  return true;
}

void peripherals_init() {
  // Set max clock speed: 133MHz for RP2040
  bool clock_ok = set_sys_clock_khz(133000, true);
  if (!clock_ok) {
    while (true) {
      tight_loop_contents();
    }
  }

  stdio_init_all();
  sleep_ms(3000);

  // LED initialisation
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  // Heartbeat - toggle LED every 500ms and print a message to the console
  // (UART0: GPIO0 and GPIO1)
  add_repeating_timer_ms(500, timer_heartbeat_callback, NULL, &timer);

  // Initialize W5500 and network
  wizchip_spi_initialize();
  wizchip_cris_initialize();
  wizchip_reset();
  wizchip_initialize();
  wizchip_check();
  network_initialize(g_net_info);
  print_network_information(g_net_info);
}

int main() {
  peripherals_init();

  printf("System Clock Frequency is %d Hz\r\n", clock_get_hz(clk_sys));

  while (true) {
    tight_loop_contents();
  }
}
