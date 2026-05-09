#include "at24c02.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c1;

#define I2C_SCAN_TIMEOUT_MS     5U
#define AT24C02_WRITE_DELAY_MS  5U

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

HAL_StatusTypeDef at24c02_read_byte(uint8_t mem_addr, uint8_t *data)
{
  return HAL_I2C_Mem_Read(&hi2c1, AT24C02_ADDR, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

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

HAL_StatusTypeDef at24c02_read_bytes(uint8_t mem_addr, uint8_t *data, uint8_t len)
{
  return HAL_I2C_Mem_Read(&hi2c1, AT24C02_ADDR, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, data, len, 100);
}