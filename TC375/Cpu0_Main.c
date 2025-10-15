#include "Cpu0_Init.h"

#include "Ifx_Lwip.h"

#include "my_stdio.h"
#include "tof.h"
#include "ultrasonic.h"

#include "emer_alert.h"

void core0_main (void)
{
    if (!core0_init())
        return;

    while (true)
    {
        /* Process data queues */
        ToF_ProcessQueue();
        Ultrasonic_ProcessQueue();

        /* Check ethernet data & process */
        Ifx_Lwip_pollTimerFlags(); /* Poll LwIP timers and trigger protocols execution if required */
        Ifx_Lwip_pollReceiveFlags(); /* Receive data package through ETH */

        /* Run emer alert */
        EmerAlert_Update_Periodic();
    }
}
