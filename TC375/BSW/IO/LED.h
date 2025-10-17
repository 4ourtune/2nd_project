#ifndef BSW_IO_LED_H_
#define BSW_IO_LED_H_

#include <stdbool.h>

typedef enum
{
    LED_BACK = 0, LED_FRONT_DOWN, LED_FRONT_UP
} LedSide;

void LED_Init (void);
void LED_Toggle (LedSide side);
void LED_On (LedSide side);
void LED_Off (LedSide side);

#endif /* BSW_IO_LED_H_ */
