#include "i2c_test.h"
#include "debug_uart.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c1;

#define I2C_TEST_PERIOD_MS      3000U
#define I2C_SCAN_TIMEOUT_MS     5U
#define AT24C02_WRITE_DELAY_MS  5U

static uint32_t g_i2c_test_tick = 0U;
static uint8_t  g_test_counter  = 0U;

/**
 * @brief 扫描I2C总线上的设备地址
 * 
 * @param addr_list 存储找到的设备地址的数组
 * @param max_count 数组的最大容量
 * @return uint8_t 找到的设备数量
 */
uint8_t i2c_scan_bus(uint8_t *addr_list, uint8_t max_count)
{
  uint8_t found = 0U;

  for (uint8_t addr = 0x02U; addr < 0xFEU; addr += 2U)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr, 1, I2C_SCAN_TIMEOUT_MS) == HAL_OK)
    {
      if (found < max_count)
      {
        addr_list[found] = addr;
      }
      found++;
    }
  }
  return found;
}

/**
 * @brief 向AT24C02 EEPROM的指定内存地址写入一个字节
 * 
 * @param mem_addr 内存地址
 * @param data 要写入的数据
 * @return HAL_StatusTypeDef 操作状态，成功返回HAL_OK
 */
HAL_StatusTypeDef at24c02_write_byte(uint8_t mem_addr, uint8_t data)
{
  HAL_StatusTypeDef ret;
  ret = HAL_I2C_Mem_Write(&hi2c1, AT24C02_ADDR, mem_addr,
                           I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  if (ret == HAL_OK)
  {
    HAL_Delay(AT24C02_WRITE_DELAY_MS);
  }
  return ret;
}

/**
 * @brief 从AT24C02 EEPROM的指定内存地址读取一个字节
 * 
 * @param mem_addr 内存地址
 * @param data 指向存储读取数据的变量的指针
 * @return HAL_StatusTypeDef 操作状态，成功返回HAL_OK
 */
HAL_StatusTypeDef at24c02_read_byte(uint8_t mem_addr, uint8_t *data)
{
  return HAL_I2C_Mem_Read(&hi2c1, AT24C02_ADDR, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

/**
 * @brief 向AT24C02 EEPROM的指定内存地址写入多个字节
 * 
 * @param mem_addr 起始内存地址
 * @param data 要写入的数据数组
 * @param len 要写入的字节数
 * @return HAL_StatusTypeDef 操作状态，成功返回HAL_OK
 */
HAL_StatusTypeDef at24c02_write_bytes(uint8_t mem_addr, const uint8_t *data, uint8_t len)
{
  uint8_t remaining = len;
  uint8_t offset = 0U;

  while (remaining > 0U)
  {
    uint8_t page_offset = (mem_addr + offset) % AT24C02_PAGE_SIZE;
    uint8_t chunk = AT24C02_PAGE_SIZE - page_offset;
    if (chunk > remaining)
    {
      chunk = remaining;
    }

    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Write(&hi2c1, AT24C02_ADDR, mem_addr + offset,
                             I2C_MEMADD_SIZE_8BIT,
                             (uint8_t *)&data[offset], chunk, 100);
    if (ret != HAL_OK)
    {
      return ret;
    }
    HAL_Delay(AT24C02_WRITE_DELAY_MS);

    offset += chunk;
    remaining -= chunk;
  }
  return HAL_OK;
}

/**
 * @brief 从AT24C02 EEPROM的指定内存地址读取多个字节
 * 
 * @param mem_addr 起始内存地址
 * @param data 存储读取数据的缓冲区
 * @param len 要读取的字节数
 * @return HAL_StatusTypeDef 操作状态，成功返回HAL_OK
 */
HAL_StatusTypeDef at24c02_read_bytes(uint8_t mem_addr, uint8_t *data, uint8_t len)
{
  return HAL_I2C_Mem_Read(&hi2c1, AT24C02_ADDR, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

/**
 * @brief 扫描I2C总线并报告找到的设备
 * 
 * @return 无返回值
 */
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

/**
 * @brief 对AT24C02 EEPROM进行读写测试
 * 
 * @return 无返回值
 */
static void at24c02_read_write_test(void)
{
  uint8_t write_val = g_test_counter;
  uint8_t read_val  = 0U;

  debug_uart_send_string("[AT24] Write addr=0x00 val=");
  debug_uart_send_number((int32_t)write_val);

  if (at24c02_write_byte(0x00U, write_val) != HAL_OK)
  {
    debug_uart_send_line(" FAIL");
    return;
  }
  debug_uart_send_line(" OK");

  if (at24c02_read_byte(0x00U, &read_val) != HAL_OK)
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

/**
 * @brief 初始化I2C测试模块
 * 
 * @return 无返回值
 */
void i2c_test_init(void)
{
  g_i2c_test_tick = HAL_GetTick();
  g_test_counter = 0U;

  debug_uart_send_line("[I2C] Stage7 I2C test init");
  i2c_scan_and_report();
}

/**
 * @brief I2C测试任务处理函数
 * 
 * @return 无返回值
 */
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