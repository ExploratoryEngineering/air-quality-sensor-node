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

You'll need MCUBoot and a key for the application image. If you change the key
you can't deploy the image on existing devices with a different key. A new key
means you'll have to reinstall the bootloader on all your devices.

### Initial setup

Install the "reto" tool:

`go get -u https://github.com/ExploratoryEngineering/reto`

Ensure your Zephyr installation is up and running by running `west build`.

Create a new key and install the bootloader on your device:

```shell
make fwkey
make build_mcuboot
make install_mcuboot
```

### Installing application image

This will rebuild and flash the application image:

```shell
make flash
```
