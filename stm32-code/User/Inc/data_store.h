#ifndef DATA_STORE_H
#define DATA_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef struct
{
  uint8_t  plant_type;
  uint16_t soil_low;
  uint16_t soil_high;
  uint8_t  temp_low;
  uint8_t  temp_high;
} threshold_config_t;

void data_store_init(void);
void data_store_task(void);
void data_store_dump(void);
void data_store_clear(void);

void data_store_load_thresholds(threshold_config_t *cfg);
void data_store_save_thresholds(const threshold_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* DATA_STORE_H */