#ifndef BSW_CONTROLLER_EMER_ALERT_TYPES_H_
#define BSW_CONTROLLER_EMER_ALERT_TYPES_H_

#include <stdint.h>

typedef struct
{
    int64_t interval_ms;
    uint64_t output_time_us;
} EmerAlertData_t;

#endif /* BSW_CONTROLLER_EMER_ALERT_TYPES_H_ */
