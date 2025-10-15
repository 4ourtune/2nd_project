#ifndef BSW_IO_TOF_H_
#define BSW_IO_TOF_H_

#include <stdbool.h>
#include "tof_types.h"

// 초기화
bool ToF_Init (int buffer_size, int max_bytes);

// 인터럽트에서 호출
void ToF_RxHandler (uint8_t byte);

// 메인 루프에서 호출
void ToF_ProcessQueue (void);

// 최신 데이터 가져오기
bool ToF_GetLatestData (ToFData_t *out);

#endif /* BSW_IO_TOF_H_ */
