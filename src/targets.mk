## Copyright (c) 2016, Devan Lai
##
## Permission to use, copy, modify, and/or distribute this software
## for any purpose with or without fee is hereby granted, provided
## that the above copyright notice and this permission notice
## appear in all copies.
##
## THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
## WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
## AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
## CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
## LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
## NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
## CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

ifeq ($(TARGET),STM32F042)
	TARGET_COMMON_DIR	:= ./stm32f042
	TARGET_SPEC_DIR		:= ./stm32f042
	LDSCRIPT			?= ./stm32f042/stm32f042x6.ld
	ARCH				= STM32F0
endif
ifeq ($(TARGET),STLINK)
	TARGET_COMMON_DIR	:= ./stm32f103
	TARGET_SPEC_DIR		:= ./stm32f103/stlink
	LDSCRIPT			?= ./stm32f103/stm32f103x8.ld
	ARCH				= STM32F1
endif
ifeq ($(TARGET),STLINK-DFUBOOT)
	TARGET_COMMON_DIR	:= ./stm32f103
	TARGET_SPEC_DIR		:= ./stm32f103/stlink
	LDSCRIPT			?= ./stm32f103/stm32f103x8-dfuboot.ld
	ARCH				= STM32F1
endif
ifeq ($(TARGET),BLUEPILL)
	TARGET_COMMON_DIR	:= ./stm32f103
	TARGET_SPEC_DIR		:= ./stm32f103/bluepill
	LDSCRIPT			?= ./stm32f103/stm32f103x8.ld
	ARCH				= STM32F1
endif
ifeq ($(TARGET),BLUEPILL-DFUBOOT)
	TARGET_COMMON_DIR	:= ./stm32f103
	TARGET_SPEC_DIR		:= ./stm32f103/bluepill
	LDSCRIPT			?= ./stm32f103/stm32f103x8-dfuboot.ld
	ARCH				= STM32F1
endif
ifndef ARCH
$(error Unknown target $(TARGET))
endif