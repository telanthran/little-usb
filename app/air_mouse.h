#ifndef AIR_MOUSE_H_
#define AIR_MOUSE_H_

#include <stdint.h>
#include "accelerometer.h"
#include "button.h"

void air_mouse_init(AccelerometerDriver *accel_driver, ButtonDriver *button_driver);
void air_mouse_write_report(uint8_t endpoint_number);

#endif /* AIR_MOUSE_H_ */
