/**
 * Stage 7 + 8 combined demo entry point.
 *
 * Integration guide (main.c USER CODE blocks):
 *
 * 1) Add include:
 *      #include "stage7_8_demo.h"
 *
 * 2) In USER CODE BEGIN 2 (after other _init calls):
 *      stage7_8_demo_init();
 *
 * 3) In main while(1) loop (USER CODE BEGIN 3):
 *      stage7_8_demo_task();
 *
 * Prerequisites:
 *   - CubeMX must enable I2C1 (PB6 SCL, PB7 SDA)
 *   - PA4 must be configured as GPIO Output Open-Drain (for DS18B20)
 *   - USART1 must be enabled (debug_uart module)
 */

#include "stage7_8_demo.h"
#include "i2c_test.h"
#include "ds18b20.h"
#include "debug_uart.h"

void stage7_8_demo_init(void)
{
  debug_uart_send_line("========================================");
  debug_uart_send_line("  Stage 7+8 Demo: I2C + DS18B20");
  debug_uart_send_line("========================================");

  i2c_test_init();
  ds18b20_init();

  debug_uart_send_line("[DEMO] Init complete");
}

void stage7_8_demo_task(void)
{
  i2c_test_task();
  ds18b20_task();
}