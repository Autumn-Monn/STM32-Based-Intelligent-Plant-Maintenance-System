#include "debug_uart.h"
#include "usart.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

#define UART_RX_BUF_SIZE  16U

static uint8_t g_rx_byte;
static uint8_t g_rx_buf[UART_RX_BUF_SIZE];
static volatile uint8_t g_rx_head = 0U;
static volatile uint8_t g_rx_tail = 0U;

void debug_uart_init(void)
{
  HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
  debug_uart_send_line("=== Plant System Boot ===");
}

void debug_uart_send_bytes(const uint8_t *data, uint16_t len)
{
  HAL_UART_Transmit(&huart1, data, len, HAL_MAX_DELAY);
}

void debug_uart_send_string(const char *str)
{
  debug_uart_send_bytes((const uint8_t *)str, (uint16_t)strlen(str));
}

void debug_uart_send_line(const char *str)
{
  debug_uart_send_string(str);
  debug_uart_send_string("\r\n");
}

void debug_uart_send_number(int32_t num)
{
  char buf[12];
  int i = 0;
  uint32_t u;

  if (num < 0)
  {
    debug_uart_send_string("-");
    u = (uint32_t)(-(num + 1)) + 1U;
  }
  else
  {
    u = (uint32_t)num;
  }

  if (u == 0U)
  {
    debug_uart_send_string("0");
    return;
  }

  while (u > 0U)
  {
    buf[i++] = '0' + (char)(u % 10U);
    u /= 10U;
  }

  while (i > 0)
  {
    char c = buf[--i];
    debug_uart_send_bytes((const uint8_t *)&c, 1);
  }
}

void debug_uart_rx_callback(void)
{
  uint8_t next = (g_rx_head + 1U) % UART_RX_BUF_SIZE;
  if (next != g_rx_tail)
  {
    g_rx_buf[g_rx_head] = g_rx_byte;
    g_rx_head = next;
  }
  HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
}

uint8_t debug_uart_has_data(void)
{
  return (g_rx_head != g_rx_tail) ? 1U : 0U;
}

uint8_t debug_uart_read_byte(void)
{
  if (g_rx_head == g_rx_tail)
  {
    return 0U;
  }

  uint8_t data = g_rx_buf[g_rx_tail];
  g_rx_tail = (g_rx_tail + 1U) % UART_RX_BUF_SIZE;
  return data;
}