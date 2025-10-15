#ifndef BSW_IO_ULTRASONIC_H_
#define BSW_IO_ULTRASONIC_H_

#include <stdbool.h>
#include "ultrasonic_types.h"

bool Ultrasonic_Init (int buffer_size, int max_events);
void Ultrasonic_EchoHandler (UltrasonicSide side, bool input_pin_state);
void Ultrasonic_ProcessQueue (void);
bool Ultrasonic_GetLatestData (UltrasonicSide side, UltrasonicData_t *out);

#endif /* BSW_IO_ULTRASONIC_H_ */
