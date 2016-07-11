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

#ifndef CONSOLE_H_INCLUDED
#define CONSOLE_H_INCLUDED

#include <stddef.h>
#include <libopencm3/stm32/usart.h>

#include "config.h"

#if !CONSOLE_SPLIT_USART
#define CONSOLE_TX_USART CONSOLE_USART
#define CONSOLE_RX_USART CONSOLE_USART
#define CONSOLE_TX_USART_GPIO_PORT CONSOLE_USART_GPIO_PORT
#define CONSOLE_RX_USART_GPIO_PORT CONSOLE_USART_GPIO_PORT
#define CONSOLE_TX_USART_CLOCK CONSOLE_USART_CLOCK
#define CONSOLE_RX_USART_CLOCK CONSOLE_USART_CLOCK
#define CONSOLE_TX_USART_IRQ_NAME CONSOLE_USART_IRQ_NAME
#define CONSOLE_RX_USART_IRQ_NAME CONSOLE_USART_IRQ_NAME
#define CONSOLE_TX_USART_NVIC_LINE CONSOLE_USART_NVIC_LINE
#define CONSOLE_RX_USART_NVIC_LINE CONSOLE_USART_NVIC_LINE
#endif



extern void console_setup(uint32_t baudrate);
extern void console_reconfigure(uint32_t baudrate, uint32_t databits,
                                uint32_t stopbits, uint32_t parity);

inline void console_send_blocking(uint8_t data) {
    usart_send_blocking(CONSOLE_TX_USART, data);
}

inline uint8_t console_recv_blocking(void) {
    return usart_recv_blocking(CONSOLE_RX_USART);
}

extern size_t console_send_buffered(const uint8_t* data, size_t num_bytes);
extern size_t console_recv_buffered(uint8_t* data, size_t max_bytes);

extern void console_set_echo(bool enable);

#endif
