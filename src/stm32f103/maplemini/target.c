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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "target.h"
#include "config.h"

/* Set STM32 to 72 MHz. */
void clock_setup(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

void gpio_setup(void) {
    /*
      LED0 on PB1, button on PB8, USB pullup control on PB9 
      TX, RX (MCU-side) on PA2, PA3
    */

    /* Enable GPIOA and GPIOC clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Setup LEDs as open-drain outputs */
    {
        const uint8_t mode = GPIO_MODE_OUTPUT_10_MHZ;
        const uint8_t conf = (LED_OPEN_DRAIN ? GPIO_CNF_OUTPUT_OPENDRAIN
                                             : GPIO_CNF_OUTPUT_PUSHPULL);
        gpio_set_mode(GPIOB, mode, conf, GPIO1);
    }
    
    {
        const uint8_t mode = GPIO_MODE_OUTPUT_10_MHZ;
        const uint8_t conf = (USB_PULLUP_OPEN_DRAIN ? GPIO_CNF_OUTPUT_OPENDRAIN
                                                    : GPIO_CNF_OUTPUT_PUSHPULL);
        /* Configure USB pullup transistor, initially disabled */
        if (USB_PULLUP_ACTIVE_HIGH) {
            gpio_clear(USB_PULLUP_GPIO_PORT, USB_PULLUP_GPIO_PIN);
        } else {
            gpio_set(USB_PULLUP_GPIO_PORT, USB_PULLUP_GPIO_PIN);
        }
        gpio_set_mode(USB_PULLUP_GPIO_PORT, mode, conf, USB_PULLUP_GPIO_PIN);
    }
}

void target_console_init(void){
    /* Enable UART clocks */
    rcc_periph_clock_enable(CONSOLE_USART_CLOCK);

    /* Setup GPIO pins */
    gpio_set_mode(CONSOLE_USART_GPIO_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, CONSOLE_USART_GPIO_TX);
    gpio_set_mode(CONSOLE_USART_GPIO_PORT, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, CONSOLE_USART_GPIO_RX);
}

void led_bit(uint8_t position, bool state) {
    uint32_t gpio = 0xFFFFFFFFU;
    if (position == 0) {
        gpio = GPIO1;
    }

    if (gpio != 0xFFFFFFFFU) {
        if (state ^ LED_OPEN_DRAIN) {
            gpio_set(GPIOB, gpio);
        } else {
            gpio_clear(GPIOB, gpio);
        }
    }
}

void led_num(uint8_t value) {
    if ((value & 0x1) ^ LED_OPEN_DRAIN) {
        gpio_set(GPIOB, GPIO1);
    } else {
        gpio_clear(GPIOB, GPIO1);
    }
}