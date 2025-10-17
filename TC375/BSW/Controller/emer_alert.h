#ifndef BSW_CONTROLLER_EMER_ALERT_H_
#define BSW_CONTROLLER_EMER_ALERT_H_

#include <stdbool.h>

#include "emer_alert_types.h"

void EmerAlert_Update_Periodic (void);
bool EmerAlert_Set_Interval (int64_t toggle_interval_ms);
bool EmerAlert_GetLatestData (EmerAlertData_t *out);

#endif /* BSW_CONTROLLER_EMER_ALERT_H_ */
