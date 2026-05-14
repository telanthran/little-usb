#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "usb_standards.h"
#include "hid/usb_hid_standards.h"

typedef struct {
    UsbConfigurationDescriptor usb_configuration_descriptor;
    UsbInterfaceDescriptor     usb_interface_descriptor;
    UsbHidDescriptor           usb_mouse_hid_descriptor;
    UsbEndpointDescriptor      usb_mouse_endpoint_descriptor;
} UsbConfigurationDescriptorCombination;

typedef struct {
    int8_t  x;
    int8_t  y;
    uint8_t buttons;
} __attribute__((__packed__)) HidReport;

extern const UsbDeviceDescriptor                   device_descriptor;
extern const uint8_t                               hid_report_descriptor[];
extern const UsbConfigurationDescriptorCombination configuration_descriptor_combination;
extern const uint16_t hid_report_descriptor_size; // <-- Add this line

#endif /* USB_DESCRIPTORS_H_ */
