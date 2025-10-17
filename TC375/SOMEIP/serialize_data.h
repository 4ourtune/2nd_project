#ifndef SOMEIP_SERIALIZE_DATA_H_
#define SOMEIP_SERIALIZE_DATA_H_

#include <stdint.h>

#include "tof_types.h"
#include "ultrasonic_types.h"
#include "motor_controller_types.h"
#include "emer_alert_types.h"

uint16_t Serialize_ToFData (uint8_t *txBuf, uint16_t txLen, const ToFData_t *data);
uint16_t Serialize_UltrasonicData (uint8_t *txBuf, uint16_t txLen, const UltrasonicData_t *data);
uint16_t Serialize_MotorControllerData (uint8_t *txBuf, uint16_t txLen, const MotorControllerData_t *data);
uint16_t Serialize_EmerAlertData (uint8_t *txBuf, uint16_t txLen, const EmerAlertData_t *data);

#endif /* SOMEIP_SERIALIZE_DATA_H_ */
