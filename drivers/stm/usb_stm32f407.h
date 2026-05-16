#ifndef USB_STM32F407_H_
#define USB_STM32F407_H_

#include "stm32f4xx.h"
//#include "usb_standards.h"
#include "usb_hw_interface.h"

#define ENDPOINT_COUNT 4

#define USB_OTG_FS_GLOBAL   ((USB_OTG_GlobalTypeDef*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_GLOBAL_BASE))
#define USB_OTG_FS_DEVICE   ((USB_OTG_DeviceTypeDef*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define USB_OTG_FS_PCGCCTL  ((uint32_t*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE))

inline static USB_OTG_INEndpointTypeDef *IN_ENDPOINT(uint8_t endpoint_number) {
    return (USB_OTG_INEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (endpoint_number * 0x20));
}

inline static USB_OTG_OUTEndpointTypeDef *OUT_ENDPOINT(uint8_t endpoint_number) {
    return (USB_OTG_OUTEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (endpoint_number * 0x20));
}

inline static __IO uint32_t *FIFO(uint8_t endpoint_number) {
    return (__IO uint32_t *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_FIFO_BASE + (endpoint_number * 0x1000));
}

#endif /* USB_STM32F407_H_ */
