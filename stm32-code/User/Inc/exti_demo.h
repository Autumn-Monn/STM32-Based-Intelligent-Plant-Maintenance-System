#ifndef EXTI_DEMO_H
#define EXTI_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void exti_demo_init(void);
void exti_demo_task(void);
void exti_demo_gpio_exti_callback(uint16_t gpio_pin);

#ifdef __cplusplus
}
#endif

#endif /* EXTI_DEMO_H */
