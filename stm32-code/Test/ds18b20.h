#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define DS18B20_GPIO_PORT   GPIOA
#define DS18B20_GPIO_PIN    GPIO_PIN_4

typedef enum
{
  DS18B20_OK       = 0U,
  DS18B20_NO_DEV   = 1U,
  DS18B20_CRC_ERR  = 2U,
  DS18B20_TIMEOUT  = 3U
} ds18b20_status_t;

void ds18b20_init(void);
void ds18b20_task(void);

ds18b20_status_t ds18b20_read_temp(float *temperature);
ds18b20_status_t ds18b20_read_temp_raw(int16_t *raw);

int16_t  ds18b20_get_cached_raw(void);
uint8_t  ds18b20_is_valid(void);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */