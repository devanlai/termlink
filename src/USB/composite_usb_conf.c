/*
 * Copyright (c) 2015, Devan Lai
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/dfu.h>

#include "composite_usb_conf.h"
#include "usb_setup.h"

#include "misc_defs.h"

#include "dfu.h"
#include "webusb.h"

#include "config.h"

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0210,
    .bDeviceClass = USB_CLASS_MISCELLANEOUS_DEVICE,
    .bDeviceSubClass = USB_MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = USB_MISC_PROTOCOL_INTERFACE_ASSOCIATION_DESCRIPTOR,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x1209,
    .idProduct = 0x0002,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_CDC_COMM_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 16,
        .bInterval = 1,
    }
};

static const struct usb_endpoint_descriptor data_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_CDC_DATA_OUT,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_CDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ENDP_CDC_DATA_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = USB_CDC_MAX_PACKET_SIZE,
        .bInterval = 1,
    }
};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__ ((packed)) cdcacm_functional_descriptors = {
    .header = {
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength =
        sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities = 0,
        .bDataInterface = INTF_CDC_DATA,
    },
    .acm = {
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities = (1 << 1),
    },
    .cdc_union = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = INTF_CDC_COMM,
        .bSubordinateInterface0 = INTF_CDC_DATA,
    }
};

static const struct usb_iface_assoc_descriptor iface_assoc = {
    .bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
    .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface = INTF_CDC_COMM,
    .bInterfaceCount = 2,
    .bFunctionClass = USB_CLASS_CDC,
    .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
    .bFunctionProtocol = USB_CDC_PROTOCOL_NONE,
    .iFunction = 4,
};

static const struct usb_interface_descriptor comm_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_CDC_COMM,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
    .iInterface = 5,

    .endpoint = comm_endpoints,

    .extra = &cdcacm_functional_descriptors,
    .extralen = sizeof(cdcacm_functional_descriptors)
};

static const struct usb_interface_descriptor data_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_CDC_DATA,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 6,

    .endpoint = data_endpoints,
};

static const struct usb_interface_descriptor dfu_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = INTF_DFU,
    .bAlternateSetting = 0,
    .bNumEndpoints = 0,
    .bInterfaceClass = 0xFE,
    .bInterfaceSubClass = 1,
    .bInterfaceProtocol = 1,
    .iInterface = 7,

    .endpoint = NULL,

    .extra = &dfu_function,
    .extralen = sizeof(dfu_function),
};

static const struct usb_interface interfaces[] = {
    /* CDC Control Interface */
    {
        .num_altsetting = 1,
        .altsetting = &comm_iface,
        .iface_assoc = &iface_assoc
    },
    /* CDC Data Interface */
    {
        .num_altsetting = 1,
        .altsetting = &data_iface,
    },
    /* DFU interface */
    {
        .num_altsetting = 1,
        .altsetting = &dfu_iface,
    }
};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = sizeof(interfaces)/sizeof(struct usb_interface),
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xC0,
    .bMaxPower = 0x32,

    .interface = interfaces,
};

static const struct usb_device_capability_descriptor* capabilities[] = {
    (const struct usb_device_capability_descriptor*)&webusb_platform,
};

static const struct usb_bos_descriptor bos = {
    .bLength = USB_DT_BOS_SIZE,
    .bDescriptorType = USB_DT_BOS,
    .wTotalLength = USB_DT_BOS_SIZE + sizeof(webusb_platform),
    .bNumDeviceCaps = sizeof(capabilities)/sizeof(capabilities[0]),
    .capabilities = capabilities
};

static char serial_number[USB_SERIAL_NUM_LENGTH+1] = "000000000000000000000000";

static const char *usb_strings[] = {
    "Devanarchy",
    (PRODUCT_NAME " USB-Serial"),
    serial_number,
    (PRODUCT_NAME " CDC"),
    "CDC Control",
    "CDC Data",
    (PRODUCT_NAME " DFU"),
};

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128] __attribute__ ((aligned (2)));

void cmp_set_usb_serial_number(const char* serial) {
    serial_number[0] = '\0';
    if (serial) {
        strncpy(serial_number, serial, USB_SERIAL_NUM_LENGTH);
        serial_number[USB_SERIAL_NUM_LENGTH] = '\0';
    }
}

usbd_device* cmp_usb_setup(void) {
    int num_strings = sizeof(usb_strings)/sizeof(const char*);

    const usbd_driver* driver = target_usb_init();
    usbd_device* usbd_dev = usbd_init(driver, &dev, &config, &bos,
                                      usb_strings, num_strings,
                                      usbd_control_buffer, sizeof(usbd_control_buffer));

    return usbd_dev;
}
