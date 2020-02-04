# Air quality sensor node firmware

This project contains firmware for an air quality sensor node

The board is configured for sampling an Alfasense AFE-3 board populated with NO2, NO and O3 sensors, along with temperature, humidity and an OPC-N3 particulate sensor.

Building:

```sh
source <your zephyr path>/zephyr/zephyr-env.sh
export ZEPHYR_BASE=<your zephyr path>/zephyr
west build --pristine -b nrf52_pca10040 Firmware
west flash
```

## Required tools

You'll need `make` to build the image. Releases are managed via the `reto` release tool (available at github.com/ExploratoryEngineering/reto)

## Building

* `make` builds the firmware for nRF52
* `make flash` builds the firmware and flashes it to the board
