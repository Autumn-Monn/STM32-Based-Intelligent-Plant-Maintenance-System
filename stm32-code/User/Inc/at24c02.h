#ifndef AT24C02_H
#define AT24C02_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define AT24C02_ADDR        0xA0U
#define AT24C02_PAGE_SIZE   8U
#define AT24C02_TOTAL_SIZE  256U

HAL_StatusTypeDef at24c02_write_byte(uint8_t mem_addr, uint8_t data);
HAL_StatusTypeDef at24c02_read_byte(uint8_t mem_addr, uint8_t *data);
HAL_StatusTypeDef at24c02_write_bytes(uint8_t mem_addr, const uint8_t *data, uint8_t len);
HAL_StatusTypeDef at24c02_read_bytes(uint8_t mem_addr, uint8_t *data, uint8_t len);
uint8_t i2c_scan_bus(uint8_t *addr_list, uint8_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* AT24C02_H */