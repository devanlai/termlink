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

#include <libopencm3/cm3/nvic.h>

#include "console.h"
#include "target.h"

void console_setup(uint32_t baudrate) {
    /* Setup GPIO */
    target_console_init();

    usart_set_baudrate(CONSOLE_TX_USART, baudrate);
    usart_set_databits(CONSOLE_TX_USART, 8);
    usart_set_parity(CONSOLE_TX_USART, USART_PARITY_NONE);
    usart_set_stopbits(CONSOLE_TX_USART, USART_STOPBITS_1);
#if CONSOLE_SPLIT_USART
    usart_set_mode(CONSOLE_TX_USART, USART_MODE_TX);
#else
    usart_set_mode(CONSOLE_TX_USART, CONSOLE_USART_MODE);
#endif
    usart_set_flow_control(CONSOLE_TX_USART, USART_FLOWCONTROL_NONE);

    usart_enable(CONSOLE_TX_USART);
#if !CONSOLE_SPLIT_USART
    usart_enable_rx_interrupt(CONSOLE_TX_USART);
#endif

    nvic_enable_irq(CONSOLE_TX_USART_NVIC_LINE);

#if CONSOLE_SPLIT_USART
    usart_set_baudrate(CONSOLE_RX_USART, baudrate);
    usart_set_databits(CONSOLE_RX_USART, 8);
    usart_set_parity(CONSOLE_RX_USART, USART_PARITY_NONE);
    usart_set_stopbits(CONSOLE_RX_USART, USART_STOPBITS_1);
    usart_set_mode(CONSOLE_RX_USART, USART_MODE_RX);
    usart_set_flow_control(CONSOLE_RX_USART, USART_FLOWCONTROL_NONE);

    usart_enable(CONSOLE_RX_USART);
    usart_enable_rx_interrupt(CONSOLE_RX_USART);

    nvic_enable_irq(CONSOLE_RX_USART_NVIC_LINE);
#endif
}

void console_reconfigure(uint32_t baudrate, uint32_t databits, uint32_t stopbits,
                         uint32_t parity) {
    usart_disable(CONSOLE_TX_USART);
#if CONSOLE_SPLIT_USART
    usart_disable(CONSOLE_RX_USART);
#endif

    if (parity != USART_PARITY_NONE) {
        /* usart_set_databits counts parity bits as "data" bits */
        databits += 1;
    }

    usart_set_baudrate(CONSOLE_TX_USART, baudrate);
    usart_set_databits(CONSOLE_TX_USART, databits);
    usart_set_stopbits(CONSOLE_TX_USART, stopbits);
    usart_set_parity(CONSOLE_TX_USART, parity);

#if CONSOLE_SPLIT_USART
    usart_set_baudrate(CONSOLE_RX_USART, baudrate);
    usart_set_databits(CONSOLE_RX_USART, databits);
    usart_set_stopbits(CONSOLE_RX_USART, stopbits);
    usart_set_parity(CONSOLE_RX_USART, parity);
#endif
    
    usart_enable(CONSOLE_TX_USART);
#if CONSOLE_SPLIT_USART
    usart_enable(CONSOLE_RX_USART);
#endif
}

static volatile uint8_t console_tx_buffer[CONSOLE_TX_BUFFER_SIZE];
static volatile uint8_t console_rx_buffer[CONSOLE_RX_BUFFER_SIZE];

static volatile uint16_t console_tx_head = 0;
static volatile uint16_t console_tx_tail = 0;

static volatile uint16_t console_rx_head = 0;
static volatile uint16_t console_rx_tail = 0;

static bool console_tx_buffer_empty(void) {
    return console_tx_head == console_tx_tail;
}

static bool console_tx_buffer_full(void) {
    return console_tx_head == ((console_tx_tail + 1) % CONSOLE_TX_BUFFER_SIZE);
}

static void console_tx_buffer_put(uint8_t data) {
    console_tx_buffer[console_tx_tail] = data;
    console_tx_tail = (console_tx_tail + 1) % CONSOLE_TX_BUFFER_SIZE;
}

static uint8_t console_tx_buffer_get(void) {
    uint8_t data = console_tx_buffer[console_tx_head];
    console_tx_head = (console_tx_head + 1) % CONSOLE_TX_BUFFER_SIZE;
    return data;
}

static bool console_rx_buffer_empty(void) {
    return console_rx_head == console_rx_tail;
}

static bool console_rx_buffer_full(void) {
    return console_rx_head == ((console_rx_tail + 1) % CONSOLE_RX_BUFFER_SIZE);
}

static void console_rx_buffer_put(uint8_t data) {
    console_rx_buffer[console_rx_tail] = data;
    console_rx_tail = (console_rx_tail + 1) % CONSOLE_RX_BUFFER_SIZE;
}

static uint8_t console_rx_buffer_get(void) {
    uint8_t data = console_rx_buffer[console_rx_head];
    console_rx_head = (console_rx_head + 1) % CONSOLE_RX_BUFFER_SIZE;
    return data;
}

size_t console_send_buffered(const uint8_t* data, size_t num_bytes) {
    size_t bytes_written = 0;

    while (!console_tx_buffer_full() && (bytes_written < num_bytes)) {
        console_tx_buffer_put(data[bytes_written++]);
    }

    if (!console_tx_buffer_empty()) {
        usart_enable_tx_interrupt(CONSOLE_TX_USART);
    }

    return bytes_written;
}

size_t console_recv_buffered(uint8_t* data, size_t max_bytes) {
    size_t bytes_read = 0;
    while (!console_rx_buffer_empty() && (bytes_read < max_bytes)) {
        data[bytes_read++] = console_rx_buffer_get();
    }

    return bytes_read;
}

static bool console_echo_input = false;

void console_set_echo(bool enable) {
    console_echo_input = enable;
}

void CONSOLE_RX_USART_IRQ_NAME(void) {
    if (usart_get_interrupt_source(CONSOLE_RX_USART, USART_SR_RXNE)) {
        uint8_t received_byte = (uint8_t)usart_recv(CONSOLE_RX_USART);
        if (!console_rx_buffer_full()) {
            console_rx_buffer_put(received_byte);
        }

        if (console_echo_input) {
            if (received_byte == '\r') {
                console_send_buffered((const uint8_t*)"\r\n", 2);
            } else {
                console_send_buffered(&received_byte, 1);
            }
        }
    }

#if !CONSOLE_SPLIT_USART
    if (usart_get_interrupt_source(CONSOLE_TX_USART, USART_SR_TXE)) {
        if (!console_tx_buffer_empty()) {
            usart_word_t buffered_byte = console_tx_buffer_get();
            usart_send(CONSOLE_TX_USART, buffered_byte);
        } else {
            usart_disable_tx_interrupt(CONSOLE_TX_USART);
        }
    }
#endif
}

#if CONSOLE_SPLIT_USART
void CONSOLE_TX_USART_IRQ_NAME(void) {
    if (usart_get_interrupt_source(CONSOLE_TX_USART, USART_SR_TXE)) {
        if (!console_tx_buffer_empty()) {
            usart_word_t buffered_byte = console_tx_buffer_get();
            usart_send(CONSOLE_TX_USART, buffered_byte);
        } else {
            usart_disable_tx_interrupt(CONSOLE_TX_USART);
        }
    }
}
#endif
