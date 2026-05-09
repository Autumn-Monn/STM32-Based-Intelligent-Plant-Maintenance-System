#include "data_store.h"
#include "at24c02.h"
#include "ds18b20.h"
#include "soil.h"
#include "debug_uart.h"

#define STORE_MAGIC         0xA5U
#define STORE_HEADER_ADDR   0x00U
#define STORE_DATA_ADDR     0x04U
#define STORE_MAX_RECORDS   63U
#define STORE_RECORD_SIZE   4U
#define STORE_INTERVAL_MS   (5U * 60U * 1000U)

typedef struct
{
  uint8_t  magic;
  uint8_t  write_index;
  uint8_t  record_count;
  uint8_t  reserved;
} store_header_t;

static store_header_t g_header;
static uint32_t g_store_tick = 0U;

static void header_write(void)
{
  at24c02_write_bytes(STORE_HEADER_ADDR, (const uint8_t *)&g_header, 4U);
}

static void header_read(void)
{
  at24c02_read_bytes(STORE_HEADER_ADDR, (uint8_t *)&g_header, 4U);
}

void data_store_init(void)
{
  header_read();

  if (g_header.magic != STORE_MAGIC)
  {
    g_header.magic        = STORE_MAGIC;
    g_header.write_index  = 0U;
    g_header.record_count = 0U;
    g_header.reserved     = 0U;
    header_write();
    debug_uart_send_line("[STORE] Initialized (fresh)");
  }
  else
  {
    debug_uart_send_string("[STORE] Restored: ");
    debug_uart_send_number((int32_t)g_header.record_count);
    debug_uart_send_line(" records");
  }

  g_store_tick = HAL_GetTick();
}

void data_store_task(void)
{
  uint32_t now = HAL_GetTick();
  if ((now - g_store_tick) < STORE_INTERVAL_MS)
  {
    return;
  }
  g_store_tick = now;

  if (!ds18b20_is_valid())
  {
    return;
  }

  int16_t  temp = ds18b20_get_cached_raw();
  uint16_t soil = soil_read_avg(4);

  uint8_t record[STORE_RECORD_SIZE];
  record[0] = (uint8_t)(temp & 0xFFU);
  record[1] = (uint8_t)((temp >> 8) & 0xFFU);
  record[2] = (uint8_t)(soil & 0xFFU);
  record[3] = (uint8_t)((soil >> 8) & 0xFFU);

  uint8_t addr = STORE_DATA_ADDR + g_header.write_index * STORE_RECORD_SIZE;
  at24c02_write_bytes(addr, record, STORE_RECORD_SIZE);

  g_header.write_index++;
  if (g_header.write_index >= STORE_MAX_RECORDS)
  {
    g_header.write_index = 0U;
  }
  if (g_header.record_count < STORE_MAX_RECORDS)
  {
    g_header.record_count++;
  }
  header_write();

  debug_uart_send_string("[STORE] Saved #");
  debug_uart_send_number((int32_t)g_header.record_count);
  debug_uart_send_string(" temp=");
  debug_uart_send_number((int32_t)temp);
  debug_uart_send_string(" soil=");
  debug_uart_send_number((int32_t)soil);
  debug_uart_send_line("");
}

void data_store_dump(void)
{
  if (g_header.record_count == 0U)
  {
    debug_uart_send_line("[STORE] No records");
    return;
  }

  debug_uart_send_string("[STORE] Dumping ");
  debug_uart_send_number((int32_t)g_header.record_count);
  debug_uart_send_line(" records:");

  uint8_t start;
  if (g_header.record_count < STORE_MAX_RECORDS)
  {
    start = 0U;
  }
  else
  {
    start = g_header.write_index;
  }

  for (uint8_t i = 0U; i < g_header.record_count; i++)
  {
    uint8_t slot = (start + i) % STORE_MAX_RECORDS;
    uint8_t addr = STORE_DATA_ADDR + slot * STORE_RECORD_SIZE;
    uint8_t record[STORE_RECORD_SIZE];

    if (at24c02_read_bytes(addr, record, STORE_RECORD_SIZE) != HAL_OK)
    {
      debug_uart_send_line("  Read error");
      continue;
    }

    int16_t  temp = (int16_t)((uint16_t)record[0] | ((uint16_t)record[1] << 8));
    uint16_t soil = (uint16_t)record[2] | ((uint16_t)record[3] << 8);

    debug_uart_send_string("  [");
    debug_uart_send_number((int32_t)i);
    debug_uart_send_string("] temp=");
    debug_uart_send_number((int32_t)temp);
    debug_uart_send_string(" soil=");
    debug_uart_send_number((int32_t)soil);
    debug_uart_send_line("");
  }
}

void data_store_clear(void)
{
  g_header.write_index  = 0U;
  g_header.record_count = 0U;
  header_write();
  debug_uart_send_line("[STORE] Cleared");
}