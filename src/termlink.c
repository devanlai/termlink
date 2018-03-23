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

#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/iwdg.h>

#include "config.h"
#include "target.h"

#include "USB/composite_usb_conf.h"
#include "USB/cdc.h"
#include "USB/dfu.h"

#include "DFU/DFU.h"

#include "tick.h"
#include "retarget.h"
#include "console.h"

static inline uint32_t millis(void) {
    return get_ticks();
}

static inline void wait_ms(uint32_t duration_ms) {
    uint32_t now = millis();
    uint32_t end = now + duration_ms;
    if (end < now) {
        end = 0xFFFFFFFFU - end;
        while (millis() >= now) {
            __asm__("NOP");
        }
    }

    while (millis() < end) {
        __asm__("NOP");
    }
}

static uint32_t usb_timer = 0;

static void on_usb_activity(void) {
    usb_timer = 1000;
}

static bool do_reset_to_dfu = false;
static void on_dfu_request(void) {
    do_reset_to_dfu = true;
}

static void update_modem_gpio(bool dtr, bool rts) {
    target_set_dtr(dtr);
    target_set_rts(rts);
}

int main(void) {
    DFU_maybe_jump_to_bootloader();

    clock_setup();
    tick_setup(1000);
    gpio_setup();
    led_num(0);

    console_setup(DEFAULT_BAUDRATE);
    retarget(STDOUT_FILENO, CONSOLE_TX_USART);
    retarget(STDERR_FILENO, CONSOLE_TX_USART);

    led_num(1);

    {
        char serial[USB_SERIAL_NUM_LENGTH+1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH+1);
        cmp_set_usb_serial_number(serial);
    }

    usbd_device* usbd_dev = cmp_usb_setup();
    cdc_uart_app_setup(usbd_dev, &update_modem_gpio, &on_usb_activity, &on_usb_activity);

    if (DFU_AVAILABLE) {
        dfu_setup(usbd_dev, &on_dfu_request);
    }

    tick_start();

    while (1) {
        usbd_poll(usbd_dev);

        if (cdc_uart_app_update()) {
            on_usb_activity();
        }

        if (do_reset_to_dfu) {
            /* Blink 3 times to indicate reset */
            int x;
            for (x=0; x < 3; x++) {
                iwdg_reset();
                led_num(7);
                wait_ms(150);
                led_num(0);
                wait_ms(150);
                iwdg_reset();
            }

            DFU_reset_and_jump_to_bootloader();
        }

        if (usb_timer > 0) {
            usb_timer--;
            led_bit(0, 1);
        } else {
            led_bit(0, 0);
        }
    }

    return 0;
}
