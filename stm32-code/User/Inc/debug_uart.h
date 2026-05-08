#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void debug_uart_init(void);
void debug_uart_send_string(const char *str);
void debug_uart_send_bytes(const uint8_t *data, uint16_t len);
void debug_uart_send_number(int32_t num);
void debug_uart_send_line(const char *str);

uint8_t debug_uart_has_data(void);
uint8_t debug_uart_read_byte(void);
void debug_uart_rx_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_UART_H */