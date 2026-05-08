#ifndef SOIL_H
#define SOIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void soil_init(void);
uint16_t soil_read_raw(void);
uint16_t soil_read_avg(uint8_t samples);
void soil_stage6_demo(void);

#ifdef __cplusplus
}
#endif

#endif /* SOIL_H */