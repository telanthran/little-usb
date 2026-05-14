#ifndef USB_FRAMEWORK_H_
#define USB_FRAMEWORK_H_

#include <stdint.h>
#include "usb_device.h"

typedef struct {
    UsbDevice *device;
    void (*on_in_transfer_completed)(uint8_t endpoint_number);
} UsbFrameworkConfig;

void usb_framework_init(UsbFrameworkConfig *config);
void usb_framework_poll(void);

#endif /* USB_FRAMEWORK_H_ */
