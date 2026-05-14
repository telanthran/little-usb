#ifndef STM32F407_DISCOVERY_H_
#define STM32F407_DISCOVERY_H_

#include "accelerometer.h"
#include "button.h"

extern AccelerometerDriver *board_accel;
extern ButtonDriver        *board_button;

void board_init(void);

#endif /* STM32F407_DISCOVERY_H_ */
