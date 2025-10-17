#ifndef BSW_IO_PR_TYPES_H_
#define BSW_IO_PR_TYPES_H_

#include <stdint.h>

typedef struct
{
    uint32_t val;
    uint64_t received_time_us;
} PRData_t;

#endif /* BSW_IO_PR_TYPES_H_ */
