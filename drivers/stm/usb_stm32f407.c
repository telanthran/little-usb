#include "usb_stm32f407.h"
#include "string.h"
#include "logger.h"

// See Table 213. TRDT values
#define TRDT_MIN_VAL    0x9

void init_gpio_pins(void)
{
    // 1. F407 USB pins are on GPIOA (not GPIOB)
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);

    // 2. F407 USB pins are PA11 (DM) and PA12 (DP)
    // 3. For the FS core, the Alternate Function is AF10 (not AF12)
    MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL11, _VAL2FLD(GPIO_AFRH_AFSEL11, 10));
    MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL12, _VAL2FLD(GPIO_AFRH_AFSEL12, 10));

    // Set PA11 and PA12 to Alternate Function mode (Binary 10 -> Decimal 2)
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER11, _VAL2FLD(GPIO_MODER_MODER11, 2));
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER12, _VAL2FLD(GPIO_MODER_MODER12, 2));
}

void init_core(void)
{
    // 1. Enable clock for USB FS core ONLY
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_OTGFSEN);

    // 2. EVERYTHING below must point to USB_OTG_FS, never HS

    // Configure USB core to run in device mode, and use embedded full-speed PHY
    MODIFY_REG(USB_OTG_FS->GUSBCFG,
            USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL | USB_OTG_GUSBCFG_TRDT,
            USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL | _VAL2FLD(USB_OTG_GUSBCFG_TRDT, TRDT_MIN_VAL));

    // Configure device to run in full speed mode
    MODIFY_REG(USB_OTG_FS_DEVICE->DCFG,
            USB_OTG_DCFG_DSPD,
            _VAL2FLD(USB_OTG_DCFG_DSPD, 0x03));

    // Enable VBUS sensing
//    SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_VBUSBSEN);
    // Force the USB core to assume it is connected to a host
    SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_NOVBUSSENS);
    CLEAR_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_VBUSBSEN);
    CLEAR_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_VBUSASEN);

    // Unmask main USB core interrupts
    SET_BIT(USB_OTG_FS->GINTMSK,
            USB_OTG_GINTMSK_USBRST |     // Reset signal
            USB_OTG_GINTMSK_ENUMDNEM |     // Enumeration of device done
            USB_OTG_GINTMSK_SOFM |        // SOF signal detected
            USB_OTG_GINTMSK_USBSUSPM |     // USB suspended signal
            USB_OTG_GINTMSK_WUIM |         // Resume signal from suspension
            USB_OTG_GINTMSK_IEPINT |    // Interrupt raised on endpoint
            USB_OTG_GINTMSK_OEPINT |
            USB_OTG_GINTMSK_RXFLVLM        // Data in FIFO that can be read
    );

    // Clear all pending core interrupts
    WRITE_REG(USB_OTG_FS->GINTSTS, 0xFFFFFFFF);

    // Unmask USB global interrupt
    SET_BIT(USB_OTG_FS->GAHBCFG, USB_OTG_GAHBCFG_GINT);

    // Unmasks transfer completed interrupts for all endpoints
    SET_BIT(USB_OTG_FS_DEVICE->DOEPMSK, USB_OTG_DOEPMSK_XFRCM);
    SET_BIT(USB_OTG_FS_DEVICE->DIEPMSK, USB_OTG_DIEPMSK_XFRCM);
}

static void set_device_address(uint8_t address)
{
    MODIFY_REG(
        USB_OTG_FS_DEVICE->DCFG,
        USB_OTG_DCFG_DAD,
        _VAL2FLD(USB_OTG_DCFG_DAD, address)
    );
}

void connect(void)
{
    // Power on the transceivers (Must be FS)
    SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_PWRDWN);

    // Connects device to the bus by clearing Soft Disconnect (Must be FS)
    CLEAR_BIT(USB_OTG_FS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);
}

void disconnect(void)
{
    // Disconnect device from the bus (Must be FS)
    SET_BIT(USB_OTG_FS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);

    // Power off the transceivers (Must be FS)
    CLEAR_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_PWRDWN);
}

/**
 * Pops data from the RxFIFO and stores it in the buffer
 * @param buffer Pointer to the buffer, in which the popped data will be stored
 * @param size Count of bytes to be popped from the dedicated RxFIFO memory
 */
static void read_packet(void const *buffer, uint16_t size)
{
    // Note: There is only one RxFIFO
    volatile uint32_t *fifo = FIFO(0);

    for (; size >= 4; size -=4, buffer += 4) {
        // Pops one 32-bit word of data (until there is less than one word remaining)
        uint32_t data = *fifo;
        // Stores the data in the buffer
        *((uint32_t*)buffer) = data;
    }

    if (size > 0) {
        // Pops the last remaining bytes (which are less than one word)
        uint32_t data = *fifo;

        for(; size > 0; size--, buffer++, data >>= 8) {
            // Stores the data in the buffer with the correct alignment
            *((uint8_t*)buffer) = 0xFF & data;
        }
    }
}

/**
 * Pushes a packet into the TxFIFO of an IN endpoint
 * @param endpoint_number The number of the endpoint, to which the data will be written
 * @param buffer Pointer to the buffer contains the data to be written to the endpoint
 * @param size The size of data to be written in bytes
 */
static void write_packet(uint8_t endpoint_number, void const *buffer, uint16_t size)
{
    volatile uint32_t *fifo = FIFO(endpoint_number);
    USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);

    // Configures the transmission (1 packet that has `size` bytes)
    MODIFY_REG(in_endpoint->DIEPTSIZ,
        USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
        _VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, size)
    );

    // Enables the transmission after clearing both STALL and NAK of the endpoint
    MODIFY_REG(in_endpoint->DIEPCTL,
        USB_OTG_DIEPCTL_STALL,
        USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA
    );

    // Gets the size in term of 32-bit words (to avoid integer overflow in the loop)
    size = (size + 3) / 4;

    for (; size > 0; size--, buffer += 4)
    {
        // Pushes the data to the TxFIFO
        *fifo = *((uint32_t *)buffer);
    }
}

/**
 * Flushes the RxFIFO of all OUT endpoints
 */
static void flush_rxfifo()
{
    SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
}

/**
 * Flushes the TxFIFO of an IN endpoint
 * @param endpoint_number The number of an IN endpoint to flush its TxFIFO
 */
static void flush_txfifo(uint8_t endpoint_number)
{
    // Sets the number of the TxFIFO to be flushed and then triggers the flush
    MODIFY_REG(USB_OTG_FS->GRSTCTL,
        USB_OTG_GRSTCTL_TXFNUM,
        _VAL2FLD(USB_OTG_GRSTCTL_TXFNUM, endpoint_number) | USB_OTG_GRSTCTL_TXFFLSH
    );
}

/**
 * Updates the start addresses of all FIFOs according to the size of each FIFO
 */
static void refresh_fifo_start_addresses(void)
{
    // The first changeable start address begins after the region of RxFIFO
    uint16_t start_address = _FLD2VAL(USB_OTG_GRXFSIZ_RXFD, USB_OTG_FS->GRXFSIZ) * 4;

    // Updates the start address of the TxFIFO0
    MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ,
        USB_OTG_TX0FSA,
        _VAL2FLD(USB_OTG_TX0FSA, start_address)
    );

    // The next start address is after where the last TxFIFO ends
    start_address += _FLD2VAL(USB_OTG_TX0FD, USB_OTG_FS->DIEPTXF0_HNPTXFSIZ) * 4;

    // Updates the start addresses of the rest TxFIFOs
    for (uint8_t txfifo_number = 0; txfifo_number < ENDPOINT_COUNT - 1; txfifo_number++) {
        MODIFY_REG(USB_OTG_FS->DIEPTXF[txfifo_number],
            USB_OTG_NPTXFSA,
            _VAL2FLD(USB_OTG_NPTXFSA, start_address)
        );

        start_address += _FLD2VAL(USB_OTG_NPTXFD, USB_OTG_FS->DIEPTXF[txfifo_number]) * 4;
    }
}

/**
 * Configures the RxFIFO of all OUT endpoints
 * @param size The size of the largest OUT endpoint in bytes
 * @note The RxFIFO is shared between all OUT endpoints
 */
static void configure_rxfifo_size(uint16_t size)
{
    // Considers the space required to save status packets in RxFIFO and gets the size in term of 32-bit words
    uint16_t space = 10 + (2 * ((size / 4) + 1));

    // Configures the depth of the FIFO
    MODIFY_REG(USB_OTG_FS->GRXFSIZ,
        USB_OTG_GRXFSIZ_RXFD,
        _VAL2FLD(USB_OTG_GRXFSIZ_RXFD, space)
    );

    refresh_fifo_start_addresses();
}

/**
 * Configures the TxFIFO of an IN endpoint
 * @param endpoint_number The number of the IN endpoint we want to configure its TxFIFO
 * @param size The size of the IN endpoint in bytes
 * @note Any change on any FIFO will update the the registers of all TxFIFOs to adapt the start offsets in the FIFO dedicated memory
 */
static void configure_txfifo_size(uint8_t endpoint_number, uint16_t size)
{
    // Gets the FIFO size in term of 32-bit words
    size = (size + 3) / 4;

    // Configures the depth of the TxFIFO
    if (endpoint_number == 0) {
        MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ,
            USB_OTG_TX0FD,
            _VAL2FLD(USB_OTG_TX0FD, size)
        );
    } else {
        MODIFY_REG(USB_OTG_FS->DIEPTXF[endpoint_number - 1],
            USB_OTG_NPTXFD,
            _VAL2FLD(USB_OTG_NPTXFD, size)
        );
    }

    refresh_fifo_start_addresses();
}

/**
 * @param endpoint_size: size of packet endpoint will tx or rx
 */
static void configure_endpoint0(uint8_t endpoint_size)
{
    // Unmask all interrupts of IN and OUT endpoint0
    SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, (1 << 0) | (1 << 16));

    // Configure max packet size, activate endpoint, and NACK endpoint
    // Activating means the endpoint is functional and can send or receive data
    // Enabling means enabling the transfer on the endpoint
    MODIFY_REG(IN_ENDPOINT(0)->DIEPCTL,
            USB_OTG_DIEPCTL_MPSIZ,
            USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK);

    // Clear NAK, and enable endpoint data transmission
    SET_BIT(OUT_ENDPOINT(0)->DOEPCTL,
            USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);

    configure_rxfifo_size(64);
    configure_txfifo_size(0, endpoint_size);
}

static void configure_in_endpoint(uint8_t endpoint_number, UsbEndpointType endpoint_type, uint16_t endpoint_size)
{
    // Unmasks all interrupts of the targeted IN endpoint
    SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, 1 << endpoint_number);

    // Activates the endpoint, sets endpoint handshake to NAK (not ready to send data), sets DATA0 packet identifier,
    // configures its type, its maximum packet size, and assigns it a TxFIFO
    MODIFY_REG(IN_ENDPOINT(endpoint_number)->DIEPCTL,
        USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP | USB_OTG_DIEPCTL_TXFNUM,
        USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK |
        _VAL2FLD(USB_OTG_DIEPCTL_EPTYP, endpoint_type) | _VAL2FLD(USB_OTG_DIEPCTL_TXFNUM, endpoint_number) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM
    );

    configure_txfifo_size(endpoint_number, endpoint_size);
}

/**
 * Deconfigures IN and OUT endpoints of a specific endpoint number
 * @param endpoint_number The number of the IN and OUT endpoints to deconfigure
 */
static void deconfigure_endpoint(uint8_t endpoint_number)
{
    USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);
    USB_OTG_OUTEndpointTypeDef *out_endpoint = OUT_ENDPOINT(endpoint_number);

    // Masks all interrupts of the targeted IN and OUT endpoints
    CLEAR_BIT(USB_OTG_FS_DEVICE->DAINTMSK,
        (1 << endpoint_number) | (1 << 16 << endpoint_number)
    );

    // Clears all interrupts of the endpoint
    SET_BIT(in_endpoint->DIEPINT, 0x29FF);
    SET_BIT(out_endpoint->DOEPINT, 0x715F);

    // Disables the endpoints if possible
    if (in_endpoint->DIEPCTL & USB_OTG_DIEPCTL_EPENA) {
        // Disables endpoint transmission
        SET_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_EPDIS);
    }

    // Deactivates the endpoint
    CLEAR_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_USBAEP);

    if (endpoint_number != 0) {
        if (out_endpoint->DOEPCTL & USB_OTG_DOEPCTL_EPENA) {
            // Disables endpoint transmission
            SET_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_EPDIS);
        }

        // Deactivates the endpoint
        CLEAR_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_USBAEP);
    }

    // Flushes the FIFOs
    flush_txfifo(endpoint_number);
    // Below can be improved
    flush_rxfifo();
}

static void usbrst_handler(void)
{
    log_info("USB reset signal was detected.");

    // Stop communication on all endpoints and disable
    for (uint8_t i=0; i <= ENDPOINT_COUNT; i++) {
        deconfigure_endpoint(i);
    }

    usb_events.on_usb_reset_received();
}

static void enumdne_handler()
{
    log_info("USB device speed enumeration done.");
    configure_endpoint0(64);
}

static void rxflvl_handler()
{
     // Pop one status information word from the RxFIFO
    uint32_t receive_status = USB_OTG_FS_GLOBAL->GRXSTSP;

    // The endpoint that received the data
    uint8_t endpoint_number = _FLD2VAL(USB_OTG_GRXSTSP_EPNUM, receive_status);
    // The count of bytes in the received packet
    uint16_t bcnt = _FLD2VAL(USB_OTG_GRXSTSP_BCNT, receive_status);
    // The status of the received packet
    uint16_t pktsts = _FLD2VAL(USB_OTG_GRXSTSP_PKTSTS, receive_status);

    // TODO: replace magic numbers
    switch (pktsts) {
    case 0x06: // SETUP packet (includes data)
         usb_events.on_setup_data_received(endpoint_number, bcnt);
        break;
    case 0x02: // OUT packet (includes data)
        // TODO
        break;
    case 0x04: // SETUP stage has completed
        // Re-enables the transmission on the endpoint
        SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL,
            USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
        break;
    case 0x03: // OUT transfer has completed
        // Re-enables the transmission on the endpoint
        SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL,
            USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
        break;
    }
}

/**
 * Handles the interrupt raised when an IN endpoint has a raised interrupt
 */
static void iepint_handler()
{
    // Finds the endpoint caused the interrupt
    uint8_t endpoint_number = ffs(USB_OTG_FS_DEVICE->DAINT) - 1;

    if (IN_ENDPOINT(endpoint_number)->DIEPINT & USB_OTG_DIEPINT_XFRC)
    {
        usb_events.on_in_transfer_completed(endpoint_number);
        // Clears the interrupt flag
        SET_BIT(IN_ENDPOINT(endpoint_number)->DIEPINT, USB_OTG_DIEPINT_XFRC);
    }
}

/**
 * Handles the interrupt raised when an OUT endpoint has a raised interrupt
 */
static void oepint_handler()
{
    // Finds the endpoint caused the interrupt
    uint8_t endpoint_number = ffs(USB_OTG_FS_DEVICE->DAINT >> 16) - 1;

    if (OUT_ENDPOINT(endpoint_number)->DOEPINT & USB_OTG_DOEPINT_XFRC)
    {
        usb_events.on_out_transfer_completed(endpoint_number);
        // Clears the interrupt
        SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPINT, USB_OTG_DOEPINT_XFRC);
    }
}

void gintsts_handler(void)
{
    volatile uint32_t gintsts = USB_OTG_FS_GLOBAL->GINTSTS;

    if (gintsts & USB_OTG_GINTSTS_USBRST) {
        usbrst_handler();
        // Clear the interrupt
        SET_BIT(USB_OTG_FS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_USBRST);
    } else if (gintsts & USB_OTG_GINTSTS_ENUMDNE) {
        enumdne_handler();
        // Clears the interrupt
        SET_BIT(USB_OTG_FS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_ENUMDNE);
    } else if (gintsts & USB_OTG_GINTSTS_RXFLVL) {
        rxflvl_handler();
        // Clears the interrupt

        // I'm not sure if this is necessary
        // SET_BIT(USB_OTG_FS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_RXFLVL);
    } else if (gintsts & USB_OTG_GINTSTS_IEPINT) {
        iepint_handler();
        // Clears the interrupt
        SET_BIT(USB_OTG_FS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_IEPINT);
    } else if (gintsts & USB_OTG_GINTSTS_OEPINT) {
        oepint_handler();
        // Clears the interrupt
        SET_BIT(USB_OTG_FS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_OEPINT);
    }

    usb_events.on_usb_polled();
}

const UsbDriver_t usb_driver = {
    .init_core = &init_core,
    .init_gpio_pins = &init_gpio_pins,
    .set_device_address = &set_device_address,
    .connect = &connect,
    .disconnect = &disconnect,
    .flush_rxfifo = &flush_rxfifo,
    .flush_txfifo = &flush_txfifo,
    .configure_in_endpoint = &configure_in_endpoint,
    .read_packet = &read_packet,
    .write_packet = &write_packet,
    .poll = &gintsts_handler
};
