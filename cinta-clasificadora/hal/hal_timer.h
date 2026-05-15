// ==========================================
// HAL/hal_timer.h
// ==========================================
#ifndef HAL_TIMER_H_
#define HAL_TIMER_H_

#include <stdint.h>

void HAL_Timer0_Init(void);
uint32_t HAL_GetMillis(void);
uint32_t HAL_GetMicros(void);

#endif /* HAL_TIMER_H_ */