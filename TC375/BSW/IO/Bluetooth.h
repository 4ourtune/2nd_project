#ifndef BSW_IO_BLUETOOTH_H_
#define BSW_IO_BLUETOOTH_H_

#include <stdbool.h>
#include "bluetooth_types.h"

// 초기화
bool Bluetooth_Init (int buffer_size, int max_bytes);

// 인터럽트에서 호출
void Bluetooth_RxHandler (uint8_t byte);

// 메인 루프에서 호출
void Bluetooth_ProcessQueue (void);

// 최신 데이터 가져오기
bool Bluetooth_GetLatestData (BluetoothData_t *out);

#endif /* BSW_IO_BLUETOOTH_H_ */
