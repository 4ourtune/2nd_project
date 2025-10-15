#ifndef BSW_CONTROLLER_MOTOR_CONTROLLER_TYPES_H_
#define BSW_CONTROLLER_MOTOR_CONTROLLER_TYPES_H_

#include <stdint.h>

typedef struct
{
    int32_t motorChA_speed;
    int32_t motorChB_speed;
    uint64_t output_time_us;
} MotorControllerData_t;

#endif /* BSW_CONTROLLER_MOTOR_CONTROLLER_TYPES_H_ */
