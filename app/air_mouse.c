#include "air_mouse.h"
#include "usb_descriptors.h"
#include "usb_hw_interface.h"
#include <math.h>

static AccelerometerDriver *accel;
static ButtonDriver        *button;

void air_mouse_init(AccelerometerDriver *accel_driver, ButtonDriver *button_driver)
{
    accel  = accel_driver;
    button = button_driver;
    accel->init();
    button->init();
}

void air_mouse_write_report(uint8_t endpoint_number)
{
    if (endpoint_number != (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F)) {
        return;
    }

    float raw_x = accel->read_x_axis();
    float raw_y = accel->read_y_axis();

    int8_t mouse_x = 0;
    int8_t mouse_y = 0;
    const float DEADZONE = 10.0f;

    float magnitude = sqrtf((raw_x * raw_x) + (raw_y * raw_y));

    if (magnitude > DEADZONE) {
        float scaled = (magnitude - DEADZONE) / 2.5f;
        mouse_x = (int8_t)((raw_x / magnitude) * scaled);
        mouse_y = (int8_t)-((raw_y / magnitude) * scaled);
    }

    HidReport hid_report = {
        .x       = mouse_x,
        .y       = mouse_y,
        .buttons = button->is_pressed(),
    };

    usb_driver.write_packet(
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F),
        &hid_report,
        sizeof(hid_report)
    );
}
