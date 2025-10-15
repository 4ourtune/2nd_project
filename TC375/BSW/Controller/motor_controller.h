#ifndef BSW_CONTROLLER_MOTOR_CONTROLLER_H_
#define BSW_CONTROLLER_MOTOR_CONTROLLER_H_

#include <stdbool.h>

#include "motor_controller_types.h"

bool MotorController_GetLatestData (MotorControllerData_t *out);
bool MotorController_ProcessJoystickInput (int x, int y);
bool MotorController_ProcessWASDInput (char key);

#endif /* BSW_CONTROLLER_MOTOR_CONTROLLER_H_ */
