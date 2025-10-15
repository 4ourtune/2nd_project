#ifndef BSW_DRIVER_STM_H_
#define BSW_DRIVER_STM_H_

#include <stdbool.h>
#include <stdint.h>

uint64_t getTime10ns (void);
uint64_t getTimeUs (void);
uint64_t getTimeMs (void);

void Stm0_Init (void);
void Stm0_InterruptAfter (uint64_t delay_us);

#endif /* BSW_DRIVER_STM_H_ */
