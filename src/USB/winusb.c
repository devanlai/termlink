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

static const struct winusb_compatible_id_feature_descriptor winusb_compatible_id_descriptor = {
    .dwLength = sizeof(struct winusb_compatible_id_feature_descriptor),
    .bcdVersion = 0x0100,
    .wIndex = 0x0004,
    .bNumSections = 1,
    .reserved0 = { 0, 0, 0, 0, 0, 0, 0 },
    .bInterfaceNumber = 0,
    .reserved1 = { 1 },
    .compatibleId = "WINUSB",
    .subCompatibleId = "",
    .reserved2 = { 0, 0, 0, 0, 0, 0}
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
    switch (req->wIndex) {
        case WINUSB_REQ_GET_COMPATIBLE_ID_FEATURE_DESCRIPTOR: {
            *buf = (uint8_t*)(&winusb_compatible_id_descriptor);
            if (*len > sizeof(winusb_compatible_id_descriptor)) {
                *len = sizeof(winusb_compatible_id_descriptor);
            }
            status = USBD_REQ_HANDLED;
            break;
        }
        case WINUSB_REQ_GET_EXTENDED_PROPERTIES_OS_FEATURE_DESCRIPTOR:
        default: {
            status = USBD_REQ_NOTSUPP;
            break;
        }
    }

    return status;
}

static void winusb_set_config(usbd_device* usbd_dev, uint16_t wValue) {
    (void)wValue;
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        winusb_control_vendor_request);
}

void winusb_setup(usbd_device* usbd_dev) {
    usbd_register_extra_string(usbd_dev, 0xEE, "MSFT100!");
    usbd_register_set_config_callback(usbd_dev, winusb_set_config);

    /* Windows probes the compatible ID before setting the configuration
       so also register the callback now */
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        winusb_control_vendor_request);
}
