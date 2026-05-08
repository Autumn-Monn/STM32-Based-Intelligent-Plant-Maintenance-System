#ifndef RELAY_H
#define RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  RELAY_PUMP = 0,
  RELAY_FAN,
  RELAY_COUNT
} relay_id_t;

void relay_init(void);
void relay_on(relay_id_t relay);
void relay_off(relay_id_t relay);
void relay_toggle(relay_id_t relay);
void relay_stage4b_demo(void);

#ifdef __cplusplus
}
#endif

#endif /* RELAY_H */