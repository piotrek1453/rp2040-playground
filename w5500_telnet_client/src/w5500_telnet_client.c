#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "network_settings.h"
#include "pico/stdlib.h"
#include "port_common.h"
#include "socket.h"
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

// Telnet client settings
#define TELNET_SOCKET_NUM 0
#define TELNET_LOCAL_PORT 50000
#define TELNET_RX_BUF_SIZE 256

static uint8_t g_telnet_rx_buf[TELNET_RX_BUF_SIZE];
static uint32_t g_last_tx_ms = 0;

// Timer for heartbeat interrupt
struct repeating_timer timer;

bool timer_heartbeat_callback(struct repeating_timer *t) {
  // toggle LED
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
  return true;
}

static int telnet_connect(void) {
  int8_t socket_ret;
  int8_t connect_ret;
  uint8_t status;
  uint32_t timeout_at;

  close(TELNET_SOCKET_NUM);

  socket_ret = socket(TELNET_SOCKET_NUM, Sn_MR_TCP, TELNET_LOCAL_PORT, 0);
  if (socket_ret != TELNET_SOCKET_NUM) {
    printf("socket() failed: %d\r\n", socket_ret);
    return -1;
  }

  connect_ret =
      connect(TELNET_SOCKET_NUM, g_telnet_server_ip, TELNET_SERVER_PORT);
  if (connect_ret != SOCK_OK) {
    printf("connect() failed: %d\r\n", connect_ret);
    close(TELNET_SOCKET_NUM);
    return -1;
  }

  timeout_at = to_ms_since_boot(get_absolute_time()) + 5000;
  while (to_ms_since_boot(get_absolute_time()) < timeout_at) {
    status = getSn_SR(TELNET_SOCKET_NUM);
    if (status == SOCK_ESTABLISHED) {
      printf("Connected to %d.%d.%d.%d:%d\r\n", g_telnet_server_ip[0],
             g_telnet_server_ip[1], g_telnet_server_ip[2],
             g_telnet_server_ip[3], TELNET_SERVER_PORT);
      return 0;
    }
    if (status == SOCK_CLOSED || status == SOCK_CLOSE_WAIT) {
      break;
    }
    sleep_ms(10);
  }

  printf("Connection timeout\r\n");
  close(TELNET_SOCKET_NUM);
  return -1;
}

static void telnet_service(void) {
  uint8_t status;
  uint32_t now_ms;
  int32_t available, recv_len, send_len;
  static const uint8_t hello_msg[] = "*IDN?\r\n";

  status = getSn_SR(TELNET_SOCKET_NUM);
  if (status == SOCK_CLOSE_WAIT || status == SOCK_CLOSED) {
    close(TELNET_SOCKET_NUM);
    sleep_ms(200);
    telnet_connect();
    return;
  }

  if (status != SOCK_ESTABLISHED) {
    return;
  }

  available = getSn_RX_RSR(TELNET_SOCKET_NUM);
  if (available > 0) {
    if (available > (TELNET_RX_BUF_SIZE - 1)) {
      available = TELNET_RX_BUF_SIZE - 1;
    }
    recv_len = recv(TELNET_SOCKET_NUM, g_telnet_rx_buf, (uint16_t)available);
    if (recv_len > 0) {
      g_telnet_rx_buf[recv_len] = '\0';
      printf("TELNET RX: %s", g_telnet_rx_buf);
    } else {
      close(TELNET_SOCKET_NUM);
      return;
    }
  }

  now_ms = to_ms_since_boot(get_absolute_time());
  if (now_ms - g_last_tx_ms >= 2000) {
    send_len =
        send(TELNET_SOCKET_NUM, (uint8_t *)hello_msg, sizeof(hello_msg) - 1);
    if (send_len > 0) {
      printf("TELNET TX: %d bytes\r\n", send_len);
    }
    g_last_tx_ms = now_ms;
  }
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
  // Heartbeat - toggle LED every 500ms
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

  if (telnet_connect() != 0) {
    printf("Initial telnet connect failed, retrying in loop\r\n");
  }

  while (true) {
    telnet_service();
    sleep_ms(10);
  }
}
