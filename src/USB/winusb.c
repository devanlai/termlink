/*
 * Copyright (c) 2016, Devan Lai
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

#include "usb_common.h"
#include "winusb.h"

#include "composite_usb_conf.h"

static const struct winusb_compatible_id_descriptor winusb_wcid = {
    .dwLength = (WINUSB_COMPATIBLE_ID_HEADER_SIZE +
                 2*WINUSB_COMPATIBLE_ID_FUNCTION_SECTION_SIZE),
    .bcdVersion = 0x0100,
    .wIndex = 0x0004,
    .bNumSections = 2,
    .reserved = { 0, 0, 0, 0, 0, 0, 0 },
    .functions = {
        {
            .bInterfaceNumber = INTF_CDC_COMM,
            .reserved0 = { 1 },
            .compatibleId = "",
            .subCompatibleId = "",
            .reserved1 = { 0, 0, 0, 0, 0, 0}
        },
        {
            .bInterfaceNumber = INTF_DFU,
            .reserved0 = { 1 },
            .compatibleId = "WINUSB",
            .subCompatibleId = "",
            .reserved1 = { 0, 0, 0, 0, 0, 0}
        },
    }
};

static const struct winusb_extended_property_device_interface_guid_descriptor winusb_device_interface = {
    .dwLength = 246,
    .bcdVersion = 0x0100,
    .wIndex = 0x0005,
    .wCount = 1,
    .dwSize = 136,
    .dwPropertyDataType = WINUSB_PROP_DATA_TYPE_REG_REG_MULTI_SZ,
    .wPropertyNameLength = 42,
    .bPropertyName = {'D', '\0', 'e', '\0', 'v', '\0', 'i', '\0', 'c', '\0', 'e', '\0',
                      'I', '\0', 'n', '\0', 't', '\0', 'e', '\0', 'r', '\0', 'f', '\0', 'a', '\0', 'c', '\0', 'e', '\0',
                      'G', '\0', 'U', '\0', 'I', '\0', 'D', '\0', 's', '\0', '\0', '\0'},
    .dwPropertyDataLength = 80,
    .bPropertyData = {'{', '\0', 'D', '\0', 'E', '\0', 'F', '\0', '0', '\0', 'D', '\0', '6', '\0', 'E', '\0', '1', '\0', '-','\0',
                      '7', '\0', 'C', '\0', '0', '\0', '7', '\0', '-', '\0',
                      '4', '\0', 'C', '\0', 'B', '\0', '7', '\0', '-', '\0',
                      '9', '\0', '2', '\0', '8', '\0', '5', '\0', '-', '\0',
                      '6', '\0', '3', '\0', '3', '\0', 'F', '\0', '1', '\0', '8', '\0', '3', '\0', '0', '\0', '9', '\0', '3', '\0', 'D', '\0', '7', '\0', '}', '\0', '\0', '\0'},
};

static int winusb_control_vendor_request(usbd_device *usbd_dev,
                                         struct usb_setup_data *req,
                                         uint8_t **buf, uint16_t *len,
                                         usbd_control_complete_callback* complete) {
    (void)complete;
    (void)usbd_dev;

    if (req->bRequest != WINUSB_MS_VENDOR_CODE) {
        return USBD_REQ_NEXT_CALLBACK;
    }

    int status = USBD_REQ_NOTSUPP;
    if (((req->bmRequestType & USB_REQ_TYPE_RECIPIENT) == USB_REQ_TYPE_DEVICE) &&
        (req->wIndex == WINUSB_REQ_GET_COMPATIBLE_ID_FEATURE_DESCRIPTOR)) {
        *buf = (uint8_t*)(&winusb_wcid);
        if (*len > winusb_wcid.dwLength) {
            *len = winusb_wcid.dwLength;
        }
        status = USBD_REQ_HANDLED;
    } else if (((req->bmRequestType & USB_REQ_TYPE_RECIPIENT) == USB_REQ_TYPE_INTERFACE) &&
               (req->wIndex == WINUSB_REQ_GET_EXTENDED_PROPERTIES_OS_FEATURE_DESCRIPTOR)) {
        *buf = (uint8_t*)(&winusb_device_interface);
        if (*len > winusb_device_interface.dwLength) {
            *len = winusb_device_interface.dwLength;
        }
        status = USBD_REQ_HANDLED;
    } else {
        status = USBD_REQ_NOTSUPP;
    }

    return status;
}

static void winusb_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    (void)wValue;
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_VENDOR,
        USB_REQ_TYPE_TYPE,
        winusb_control_vendor_request);
}

void winusb_setup(usbd_device* usbd_dev) {
    usbd_register_extra_string(usbd_dev, 0xEE, "MSFT100!");
    usbd_register_set_config_callback(usbd_dev, winusb_set_config);

    /* Windows probes the compatible ID before setting the configuration,
       so also register the callback now */

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_VENDOR,
        USB_REQ_TYPE_TYPE,
        winusb_control_vendor_request);
}
