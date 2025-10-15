#ifndef BSW_IO_ULTRASONIC_TYPES_H_
#define BSW_IO_ULTRASONIC_TYPES_H_

#include <stdint.h>

typedef enum
{
    ULTRASONIC_LEFT = 0, ULTRASONIC_RIGHT, ULTRASONIC_REAR, ULTRASONIC_COUNT
} UltrasonicSide;

typedef struct
{
    int32_t dist_raw_mm;
    int32_t dist_filt_mm;
    uint64_t received_time_us;
} UltrasonicData_t;

#endif /* BSW_IO_ULTRASONIC_TYPES_H_ */
