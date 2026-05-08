#ifndef CONTROL_H
#define CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  CTRL_MODE_AUTO = 0,
  CTRL_MODE_MANUAL
} ctrl_mode_t;

typedef enum
{
  CTRL_SOIL_OK = 0,
  CTRL_SOIL_DRY,
  CTRL_SOIL_WET
} ctrl_soil_level_t;

typedef enum
{
  CTRL_TEMP_OK = 0,
  CTRL_TEMP_HIGH,
  CTRL_TEMP_LOW
} ctrl_temp_level_t;

typedef struct
{
  ctrl_mode_t       mode;
  ctrl_soil_level_t soil_level;
  ctrl_temp_level_t temp_level;
  uint8_t           pump_running;
  uint8_t           fan_running;
  uint8_t           alarm_active;
} ctrl_status_t;

void control_init(void);
void control_task(void);

void control_set_mode(ctrl_mode_t mode);
ctrl_mode_t control_get_mode(void);
const ctrl_status_t *control_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_H */