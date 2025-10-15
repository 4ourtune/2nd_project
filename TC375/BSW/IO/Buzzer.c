#include "buzzer.h"

#include "gpio.h"
#include "gpt12.h"

void Buzzer_Init (void)
{
    /* Initialize */
    GPIO_InitBuzzer();
    Gpt2_Init();

    /* Set initial state */
    GPIO_SetBuzzer(false);
    Stop_Gpt12_T6();
}

void Buzzer_Buzz (void)
{
    static int cntDelay = 0;
    cntDelay = (cntDelay + 1) % 500;
    if (cntDelay == 1)
    {
        GPIO_ToggleBuzzer();
    }
}

void Buzzer_On (void)
{
    Run_Gpt12_T6();
}

void Buzzer_Off (void)
{
    Stop_Gpt12_T6();
}
