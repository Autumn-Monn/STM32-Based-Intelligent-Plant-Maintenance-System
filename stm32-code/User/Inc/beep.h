#ifndef BEEP_H
#define BEEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void beep_init(void);
void beep_on(void);
void beep_off(void);
void beep_toggle(void);
void beep_stage4a_demo(void);

#ifdef __cplusplus
}
#endif

#endif /* BEEP_H */
