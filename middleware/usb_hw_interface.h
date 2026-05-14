#ifndef USB_HW_INTERFACE_H_
#define USB_HW_INTERFACE_H_

#include "usb_standards.h"

typedef struct
{
    void (*init_core)();
    void (*init_gpio_pins)();
    void (*set_device_address)(uint8_t address);
    void (*connect)();
    void (*disconnect)();
    void (*flush_rxfifo)();
    void (*flush_txfifo)(uint8_t endpoint_number);
    void (*configure_in_endpoint)(uint8_t endpoint_number, enum UsbEndpointType endpoint_type, uint16_t endpoint_size);
    void (*read_packet)(void const *buffer, uint16_t size);
    void (*write_packet)(uint8_t endpoint_number, void const *buffer, uint16_t size);
    void (*poll)();
    // ToDO Add pointers to the other driver functions.
} UsbDriver_t;

#endif /* USB_HW_INTERFACE_H_ */
