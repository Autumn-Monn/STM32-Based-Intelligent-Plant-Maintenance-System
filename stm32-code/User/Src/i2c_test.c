#include "i2c_test.h"
#include "at24c02.h"
#include "debug_uart.h"

#define I2C_TEST_PERIOD_MS      3000U

static uint32_t g_i2c_test_tick = 0U;
static uint8_t  g_test_counter  = 0U;

static void i2c_scan_and_report(void)
{
  uint8_t addrs[16];
  uint8_t count;

  debug_uart_send_line("[I2C] Scanning bus...");
  count = i2c_scan_bus(addrs, 16);

  if (count == 0U)
  {
    debug_uart_send_line("[I2C] No devices found");
    return;
  }

  debug_uart_send_string("[I2C] Found ");
  debug_uart_send_number((int32_t)count);
  debug_uart_send_line(" device(s):");

  for (uint8_t i = 0U; i < count && i < 16U; i++)
  {
    debug_uart_send_string("  0x");
    uint8_t hi = addrs[i] >> 4;
    uint8_t lo = addrs[i] & 0x0FU;
    char hex[3];
    hex[0] = (hi < 10U) ? ('0' + hi) : ('A' + hi - 10U);
    hex[1] = (lo < 10U) ? ('0' + lo) : ('A' + lo - 10U);
    hex[2] = '\0';
    debug_uart_send_line(hex);
  }
}

static void at24c02_read_write_test(void)
{
  uint8_t write_val = g_test_counter;
  uint8_t read_val  = 0U;

  debug_uart_send_string("[AT24] Write addr=0x20 val=");
  debug_uart_send_number((int32_t)write_val);

  if (at24c02_write_byte(0x20U, write_val) != HAL_OK)
  {
    debug_uart_send_line(" FAIL");
    return;
  }
  debug_uart_send_line(" OK");

  if (at24c02_read_byte(0x20U, &read_val) != HAL_OK)
  {
    debug_uart_send_line("[AT24] Read FAIL");
    return;
  }

  debug_uart_send_string("[AT24] Read back=");
  debug_uart_send_number((int32_t)read_val);

  if (read_val == write_val)
  {
    debug_uart_send_line(" PASS");
  }
  else
  {
    debug_uart_send_line(" MISMATCH");
  }

  g_test_counter++;
}

void i2c_test_init(void)
{
  g_i2c_test_tick = HAL_GetTick();
  g_test_counter = 0U;

  debug_uart_send_line("[I2C] I2C test init");
  i2c_scan_and_report();
}

void i2c_test_task(void)
{
  uint32_t now = HAL_GetTick();
  if ((now - g_i2c_test_tick) < I2C_TEST_PERIOD_MS)
  {
    return;
  }
  g_i2c_test_tick = now;

  at24c02_read_write_test();
}