#ifndef BSW_IO_TOF_TYPES_H_
#define BSW_IO_TOF_TYPES_H_

#include <stdint.h>

typedef struct
{
    uint8_t id;
    uint32_t system_time_ms;
    float distance_m;
    uint8_t distance_status;
    uint16_t signal_strength;
    uint64_t received_time_us;
} ToFData_t;

#endif /* BSW_IO_TOF_TYPES_H_ */
