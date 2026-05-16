#include "stddef.h"
#include "usb_framework.h"
#include "usb_hw_interface.h"
#include "usb_device.h"
#include "usb_descriptors.h"
#include "usb_standards.h"
#include "logger.h"
#include "arithmetic.h"

static UsbDevice *usbd_handle;
static void (*app_on_in_transfer_completed)(uint8_t endpoint_number);

void usb_framework_init(UsbFrameworkConfig *config)
{
    usbd_handle = config->device;
    app_on_in_transfer_completed = config->on_in_transfer_completed;
    usb_driver.init_gpio_pins();
    usb_driver.init_core();
    usb_driver.connect();
}

void usb_framework_poll(void)
{
    usb_driver.poll();
}

static void usb_reset_received_handler(void)
{
    log_info("USB reset signal was detected.");
    usbd_handle->in_data_size = 0;
    usbd_handle->out_data_size = 0;
    usbd_handle->configuration_value = 0;
    usbd_handle->device_state = USB_DEVICE_STATE_DEFAULT;
    usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;
    usb_driver.set_device_address(0);
}

static void usbd_configure(void)
{
    usb_driver.configure_in_endpoint(
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F),
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bmAttributes & 0x03),
        configuration_descriptor_combination.usb_mouse_endpoint_descriptor.wMaxPacketSize
    );

    usb_driver.write_packet(
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F),
        NULL,
        0
    );
}

static void process_standard_device_request(void)
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->bRequest)
    {
    case USB_STANDARD_GET_DESCRIPTOR:
        log_info("Standard Get Descriptor request received.");
        const uint8_t descriptor_type = request->wValue >> 8;
        const uint16_t descriptor_length = request->wLength;

        switch (descriptor_type)
        {
        case USB_DESCRIPTOR_TYPE_DEVICE:
            log_info("- Get Device Descriptor.");
            usbd_handle->ptr_in_buffer = &device_descriptor;
            usbd_handle->in_data_size = descriptor_length;
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
            break;
        case USB_DESCRIPTOR_TYPE_CONFIGURATION:
            log_info("- Get Configuration Descriptor.");
            usbd_handle->ptr_in_buffer = &configuration_descriptor_combination;
            usbd_handle->in_data_size = descriptor_length;
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
            break;
        }
        break;
    case USB_STANDARD_SET_ADDRESS:
        log_info("Standard Set Address request received.");
        usb_driver.set_device_address(request->wValue);
        usbd_handle->device_state = USB_DEVICE_STATE_ADDRESSED;
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_IN;
        break;
    case USB_STANDARD_SET_CONFIG:
        log_info("Standard Set Configuration request received.");
        usbd_handle->configuration_value = request->wValue;
        usbd_configure();
        usbd_handle->device_state = USB_DEVICE_STATE_CONFIGURED;
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_IN;
        break;
    }
}

static void process_class_interface_request(void)
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->bRequest) 
    {
    case USB_HID_SETIDLE:
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_IN;
        break;
    }
}

static void process_standard_interface_request(void)
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->wValue >> 8)
    {
    case USB_DESCRIPTOR_TYPE_HID_REPORT:
        usbd_handle->ptr_in_buffer = &hid_report_descriptor;
        usbd_handle->in_data_size = hid_report_descriptor_size;
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
        break;
    }
}

static void process_request(void)
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->bmRequestType & (USB_BM_REQUEST_TYPE_TYPE_MASK | USB_BM_REQUEST_TYPE_RECIPIENT_MASK)) {
    case USB_BM_REQUEST_TYPE_TYPE_STANDARD | USB_BM_REQUEST_TYPE_RECIPIENT_DEVICE:
        process_standard_device_request();
        break;
    case USB_BM_REQUEST_TYPE_TYPE_CLASS | USB_BM_REQUEST_TYPE_RECIPIENT_INTERFACE:
        process_class_interface_request();
        break;
    case USB_BM_REQUEST_TYPE_TYPE_STANDARD | USB_BM_REQUEST_TYPE_RECIPIENT_INTERFACE:
        process_standard_interface_request();
        break;
    }
}

static void process_control_transfer_stage(void)
{
    switch (usbd_handle->control_transfer_stage)
    {
    case USB_CONTROL_STAGE_SETUP:
        break;
    case USB_CONTROL_STAGE_DATA_IN:
        log_info("Processing IN-DATA stage.");
        uint8_t data_size = MIN(usbd_handle->in_data_size, device_descriptor.bMaxPacketSize0);
        usb_driver.write_packet(0, usbd_handle->ptr_in_buffer, data_size);
        usbd_handle->in_data_size -= data_size;
        usbd_handle->ptr_in_buffer += data_size;
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN_IDLE;

        if (usbd_handle->in_data_size == 0) {
            if (data_size == device_descriptor.bMaxPacketSize0)
                usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN_ZERO;
            else
                usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_OUT;
        }
        break;
    case USB_CONTROL_STAGE_DATA_IN_IDLE:
        break;
    case USB_CONTROL_STAGE_STATUS_OUT:
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;
        break;
    case USB_CONTROL_STAGE_STATUS_IN:
        usb_driver.write_packet(0, NULL, 0);
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;
        break;
    case USB_CONTROL_STAGE_DATA_OUT:
    case USB_CONTROL_STAGE_DATA_IN_ZERO:
        break;
    default:
        break;
    }
}

static void usb_polled_handler(void)
{
    process_control_transfer_stage();
}

static void in_transfer_completed_handler(uint8_t endpoint_number)
{
    if (usbd_handle->in_data_size) {
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
    }
    else if (usbd_handle->control_transfer_stage == USB_CONTROL_STAGE_DATA_IN_ZERO) {
        usb_driver.write_packet(0, NULL, 0);
        usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_OUT;
    }

    if (app_on_in_transfer_completed) {
        app_on_in_transfer_completed(endpoint_number);
    }
}

static void out_transfer_completed_handler(uint8_t endpoint_number)
{
    (void)endpoint_number;
}

static void setup_data_received_handler(uint8_t endpoint_number, uint16_t byte_count)
{
    usb_driver.read_packet(usbd_handle->ptr_out_buffer, byte_count);
    log_debug_array("SETUP data: ", usbd_handle->ptr_out_buffer, byte_count);
    process_request();
}

UsbEvents usb_events = {
    .on_usb_reset_received    = &usb_reset_received_handler,
    .on_setup_data_received   = &setup_data_received_handler,
    .on_usb_polled            = &usb_polled_handler,
    .on_in_transfer_completed = &in_transfer_completed_handler,
    .on_out_transfer_completed = &out_transfer_completed_handler,
};
