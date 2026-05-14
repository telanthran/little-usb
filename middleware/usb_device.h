#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

#include "usb_standards.h"

typedef struct
{
    UsbDeviceState device_state; // The current USB device state.
    UsbControlTransferStage control_transfer_stage; // The current control transfer stage (for endpoint0).
    uint8_t configuration_value; // The selected USB configuration.
    void const *ptr_out_buffer;
    uint32_t out_data_size;
    void const *ptr_in_buffer;
    uint32_t in_data_size;

} UsbDevice;

#endif /* USB_DEVICE_H_ */
