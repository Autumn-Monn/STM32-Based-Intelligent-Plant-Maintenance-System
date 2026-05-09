#ifndef DATA_STORE_H
#define DATA_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void data_store_init(void);
void data_store_task(void);
void data_store_dump(void);
void data_store_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* DATA_STORE_H */