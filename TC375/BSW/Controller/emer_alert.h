#ifndef BSW_CONTROLLER_EMER_ALERT_H_
#define BSW_CONTROLLER_EMER_ALERT_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int64_t interval_ms;
    uint64_t output_time_us;
} EmerAlertData_t;

void EmerAlert_Update_Periodic (void);
bool EmerAlert_Set_Interval (int64_t toggle_interval_ms);
bool EmerAlert_GetLatestData (EmerAlertData_t *out);

#endif /* BSW_CONTROLLER_EMER_ALERT_H_ */
