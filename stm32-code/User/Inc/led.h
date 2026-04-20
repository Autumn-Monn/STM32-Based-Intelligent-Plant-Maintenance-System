#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  LED_BOARD = 0,
  LED_GREEN,
  LED_RED,
  LED_BLUE,
  LED_COUNT
} led_id_t;

void led_init(void);
void led_on(led_id_t led);
void led_off(led_id_t led);
void led_toggle(led_id_t led);
void led_stage1_demo(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
