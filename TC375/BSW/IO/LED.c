#include "led.h"

#include "gpio.h"

void LED_Init (void)
{
    /* Initialize */
    GPIO_InitLed();

    /* Set initial state */
    GPIO_SetLed(1, false);
    GPIO_SetLed(2, false);
}

void LED_Toggle (void)
{
    GPIO_ToggleLed(1);
    GPIO_ToggleLed(2);
}

void LED_On (void)
{
    GPIO_SetLed(1, true);
    GPIO_SetLed(2, true);
}

void LED_Off (void)
{
    GPIO_SetLed(1, false);
    GPIO_SetLed(2, false);
}
