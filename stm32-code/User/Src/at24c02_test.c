#include "at24c02_test.h"
#include "at24c02.h"
#include "debug_uart.h"
#include <string.h>

static uint8_t g_pass_count = 0U;
static uint8_t g_test_total = 6U;

static void report(uint8_t test_num, const char *name, uint8_t passed)
{
  debug_uart_send_string("[TEST] ");
  debug_uart_send_number((int32_t)test_num);
  debug_uart_send_string("/6 ");
  debug_uart_send_string(name);
  if (passed)
  {
    debug_uart_send_line(" PASS");
    g_pass_count++;
  }
  else
  {
    debug_uart_send_line(" FAIL");
  }
}

static void clean_area(uint8_t addr, uint8_t len)
{
  for (uint8_t i = 0U; i < len; i++)
  {
    at24c02_write_byte(addr + i, 0x00U);
  }
}

static void test1_single_byte(void)
{
  uint8_t rd = 0U;
  uint8_t ok = 1U;

  if (at24c02_write_byte(0x10U, 0xABU) != HAL_OK) { ok = 0U; }
  if (ok && at24c02_read_byte(0x10U, &rd) != HAL_OK) { ok = 0U; }
  if (ok && rd != 0xABU) { ok = 0U; }

  clean_area(0x10U, 1U);
  report(1U, "Single byte R/W", ok);
}

static void test2_boundary_addr(void)
{
  const uint8_t addrs[] = {0x00U, 0x7FU, 0x80U, 0xFFU};
  uint8_t ok = 1U;

  for (uint8_t i = 0U; i < 4U; i++)
  {
    uint8_t rd = 0U;
    if (at24c02_write_byte(addrs[i], 0x55U) != HAL_OK) { ok = 0U; break; }
    if (at24c02_read_byte(addrs[i], &rd) != HAL_OK)    { ok = 0U; break; }
    if (rd != 0x55U) { ok = 0U; break; }
    at24c02_write_byte(addrs[i], 0x00U);
  }

  report(2U, "Boundary addresses", ok);
}

static void test3_data_patterns(void)
{
  const uint8_t patterns[] = {0x00U, 0xFFU, 0xAAU, 0x55U};
  uint8_t ok = 1U;

  for (uint8_t i = 0U; i < 4U; i++)
  {
    uint8_t rd = 0U;
    if (at24c02_write_byte(0x10U, patterns[i]) != HAL_OK) { ok = 0U; break; }
    if (at24c02_read_byte(0x10U, &rd) != HAL_OK)          { ok = 0U; break; }
    if (rd != patterns[i]) { ok = 0U; break; }
  }

  clean_area(0x10U, 1U);
  report(3U, "Data patterns", ok);
}

static void test4_page_multi_byte(void)
{
  uint8_t wr[8] = {0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U, 0x70U, 0x80U};
  uint8_t rd[8];
  uint8_t ok = 1U;

  if (at24c02_write_bytes(0x08U, wr, 8U) != HAL_OK) { ok = 0U; }
  if (ok && at24c02_read_bytes(0x08U, rd, 8U) != HAL_OK) { ok = 0U; }
  if (ok && memcmp(wr, rd, 8U) != 0) { ok = 0U; }

  clean_area(0x08U, 8U);
  report(4U, "Page multi-byte", ok);
}

static void test5_cross_page(void)
{
  uint8_t wr[10] = {0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U,
                    0xA6U, 0xA7U, 0xA8U, 0xA9U, 0xAAU};
  uint8_t rd[10];
  uint8_t ok = 1U;

  if (at24c02_write_bytes(0x05U, wr, 10U) != HAL_OK) { ok = 0U; }
  if (ok && at24c02_read_bytes(0x05U, rd, 10U) != HAL_OK) { ok = 0U; }
  if (ok && memcmp(wr, rd, 10U) != 0) { ok = 0U; }

  clean_area(0x05U, 10U);
  report(5U, "Cross-page write", ok);
}

static void test6_header_area(void)
{
  uint8_t wr[4] = {0xA5U, 0x01U, 0x02U, 0x00U};
  uint8_t rd[4];
  uint8_t ok = 1U;

  if (at24c02_write_bytes(0x00U, wr, 4U) != HAL_OK) { ok = 0U; }
  if (ok && at24c02_read_bytes(0x00U, rd, 4U) != HAL_OK) { ok = 0U; }
  if (ok && memcmp(wr, rd, 4U) != 0) { ok = 0U; }

  clean_area(0x00U, 4U);
  report(6U, "Header area", ok);
}

void at24c02_run_all_tests(void)
{
  g_pass_count = 0U;

  debug_uart_send_line("========== AT24C02 Test Suite ==========");

  test1_single_byte();
  test2_boundary_addr();
  test3_data_patterns();
  test4_page_multi_byte();
  test5_cross_page();
  test6_header_area();

  debug_uart_send_string("[TEST] Result: ");
  debug_uart_send_number((int32_t)g_pass_count);
  debug_uart_send_string("/");
  debug_uart_send_number((int32_t)g_test_total);
  if (g_pass_count == g_test_total)
  {
    debug_uart_send_line(" ALL PASSED");
  }
  else
  {
    debug_uart_send_line(" SOME FAILED");
  }
  debug_uart_send_line("========================================");
}