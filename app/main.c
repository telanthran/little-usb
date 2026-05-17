#include "logger.h"
#include "usb_framework.h"
#include "usb_device.h"
#include "air_mouse.h"
#include "stm32f407_discovery.h"

UsbDevice usb_device;
uint32_t buffer[8];
int __errno;

int main(void)
{
    usb_device.ptr_out_buffer = &buffer;

    board_init();
    air_mouse_init(board_accel, board_button);

    UsbFrameworkConfig config = {
        .device = &usb_device,
        .on_in_transfer_completed = air_mouse_write_report,
    };
    usb_framework_init(&config);

    for (;;)
    {
        usb_framework_poll();
    }
}
