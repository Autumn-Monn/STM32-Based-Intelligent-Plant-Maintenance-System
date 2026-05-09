#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define OLED_WIDTH   128U
#define OLED_HEIGHT  64U
#define OLED_PAGES   8U
#define OLED_ADDR    0x78U

typedef enum
{
  HZ_WEN = 0,
  HZ_DU,
  HZ_SHI,
  HZ_COUNT
} hz_index_t;

void oled_init(void);
void oled_clear(void);
void oled_refresh(void);

void oled_show_char(uint8_t row, uint8_t col, char ch);
void oled_show_string(uint8_t row, uint8_t col, const char *str);
void oled_show_hz(uint8_t row, uint8_t col, hz_index_t index);
void oled_show_number(uint8_t row, uint8_t col, int32_t num);

void oled_display_task(void);

#ifdef __cplusplus
}
#endif

#endif /* OLED_H */