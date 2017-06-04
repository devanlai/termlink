termlink
--------
termlink is a simple CDC-ACM USB-serial application for the STM32F103 that serves as a testbed for experimenting with bootloaders and WebUSB. It has been tested with baudrates up to 2.25MBaud.

## Build instructions
The default target is an STLink/v2 clone dongle.

To build other targets, you can override the
`TARGET` variable when invoking `make`.

    make clean
    make TARGET=BLUEPILL

### Targets

| Target Name | Description | TX/RX pins | Link |
| ----------- | ----------- |----------- | ---- |
|`STLINK`     | STLink/v2 hardware clones | RST/SWIM | https://wiki.paparazziuav.org/wiki/STLink#Clones |
|`STLINK-DFUBOOT` | STLink/v2 with dapboot bootloader | | |
|`BLUEPILL`   | Cheap dev board | PA9/PA10 | http://wiki.stm32duino.com/index.php?title=Blue_Pill |
|`BLUEPILL-DFUBOOT` | Cheap dev board with dapboot bootloader | | |

## Flash instructions
### Flashing over SWD
The `make flash` target will use openocd to upload the firmware over SWD. By default, the Makefile assumes you're using a [CMSIS-DAP](http://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php) based probe, but you can override this by overriding `OOCD_INTERFACE` variable. For example:

    make OOCD_INTERFACE=interface/stlink-v2.cfg flash

### Flashing via USB DFU
If a USB DFU bootloader such as [dapboot](https://github.com/devanlai/dapboot) has already been flashed, you can update the firmware with [dfu-util](http://dfu-util.sourceforge.net/) using the `make dfu-flash` target.

### Flashing via WebUSB
As an experimental alternative to dfu-util, you can talk to the USB DFU bootloader from the browser using WebUSB using the [webdfu demo page](https://devanlai.github.io/webdfu/dfu-util/).

Note: this has only been reliably tested with Chrome on Linux.

## Overriding defaults
Local makefile settings can be set by creating a `local.mk`, which is automatically included.

Here is an example `local.mk` that changes the default target to an STLink with a DFU bootloader

    TARGET ?= STLINK-DFUBOOT
    OOCD_INTERFACE ?= interface/stlink-v2.cfg

## USB VID/PID
The default USB VID/PID pair is [1209/0001](http://pid.codes/1209/0001/), the [pid.codes](http://pid.codes/) test PID. For personal use, it's unlikely that this will cause issues, but if distributing the firmware for wider use, you may want to reserve an appropriate PID to avoid conflicts.

## Licensing
All contents of the termlink project are licensed under terms that are compatible with the terms of the GNU Lesser General Public License version 3.

Non-libopencm3 related portions of the termlink project are licensed under the less restrictive ISC license, except where otherwise specified in the headers of specific files.

See the LICENSE file for full details.