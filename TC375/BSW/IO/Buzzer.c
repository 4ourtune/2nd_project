#include "buzzer.h"

#include "gpio.h"
#include "gpt12.h"

static int cntMax;

void Buzzer_Init (void)
{
    /* Initialize */
    GPIO_InitBuzzer();
    Gpt2_Init();

    /* Set initial state */
    cntMax = 1000;
    GPIO_SetBuzzer(false);
    Stop_Gpt12_T6();
}

bool Buzzer_SetFrequency (int f)
{
    if (f < 10 || f > 1000)
        return false;
    cntMax = f;
    return true;
}

void Buzzer_On (void)
{
    Run_Gpt12_T6();
}

void Buzzer_Off (void)
{
    Stop_Gpt12_T6();
}

void Buzzer_Buzz (void)
{
    static int cntDelay = 0;

    if (cntDelay == 0)
    {
        GPIO_ToggleBuzzer();
    }

    cntDelay = (cntDelay + 1) % cntMax;
}
