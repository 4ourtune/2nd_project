#ifndef BSW_IO_BLUETOOTH_TYPES_H_
#define BSW_IO_BLUETOOTH_TYPES_H_

#include <stdint.h>

typedef enum
{
    BLUETOOTH_CMD_INVALID = 0, BLUETOOTH_CMD_MOVE, BLUETOOTH_CMD_PARK
} BluetoothCmdType_t;

typedef struct
{
    BluetoothCmdType_t type;
    int32_t param1;
    int32_t param2;
    uint64_t received_time_us;
} BluetoothData_t;

#endif /* BSW_IO_BLUETOOTH_TYPES_H_ */
