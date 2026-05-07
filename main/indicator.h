#ifndef INDICATOR_H
#define INDICATOR_H

#include <stdint.h>
#include "board_config.h"


void indicator_init(void);
void buzzer_beep(int times, uint32_t duration_ms);
void battery_task(void *arg);

#endif