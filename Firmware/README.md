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

If you have an existing key put it into the `aq_fota.pem` file and skip this step. This is included in `.gitignore` so you won't check it in by accident.

Flash it to the device with `make flash`.

### Message format

| Length | Data type | Description |
| ------ | --------- | ----------- |
| 4 | float32 | GPS timestamp, seconds since epoch.
| 4 | float32 | GPS longitude, in radians
| 4 | float32 | GPS latitude, in radians
| 4 | float32 | GPS altitude, meters
| 4 | float32 | Board temperature, Celsius
| 4 | float32 | Board relative humidity, percent
| 4 | uint32 | OP1 ADC reading - NO2 working electrode
| 4 | uint32 | OP1 ADC reading - NO2 auxillary electrode
| 4 | uint32 | OP2 ADC reading - O3 + NO2 working electrode
| 4 | uint32 | OP2 ADC reading - O3 + NO2 auxillary electrode
| 4 | uint32 | OP3 ADC reading - NO working electrode
| 4 | uint32 | OP3 ADC reading - NO auxillary electrode
| 4 | uint32 | Pt1000 ADC reading - AFE-3 ambient temperature
| 4 | uint32 | OPC PM A (default PM1)
| 4 | uint32 | OPC PM B (default PM2.5)
| 4 | uint32 | OPC PM C (default PM10)
| 2 | uint16 | OPC sample period
| 2 | uint16 | OPC sample flowrate
| 2 | uint16 | OPC temperature
| 2 | uint16 | OPC humidity
| 2 | uint16 | OPC fan rev count
| 2 | uint16 | OPC laser status
| 2 | uint16 | OPC PM bin 1
| 2 | uint16 | OPC PM bin 2
| 2 | uint16 | OPC PM bin 3
| 2 | uint16 | OPC PM bin 4
| 2 | uint16 | OPC PM bin 5
| 2 | uint16 | OPC PM bin 6
| 2 | uint16 | OPC PM bin 7
| 2 | uint16 | OPC PM bin 8
| 2 | uint16 | OPC PM bin 9
| 2 | uint16 | OPC PM bin 10
| 2 | uint16 | OPC PM bin 11
| 2 | uint16 | OPC PM bin 12
| 2 | uint16 | OPC PM bin 13
| 2 | uint16 | OPC PM bin 14
| 2 | uint16 | OPC PM bin 15
| 2 | uint16 | OPC PM bin 16
| 2 | uint16 | OPC PM bin 17
| 2 | uint16 | OPC PM bin 18
| 2 | uint16 | OPC PM bin 19
| 2 | uint16 | OPC PM bin 20
| 2 | uint16 | OPC PM bin 21
| 2 | uint16 | OPC PM bin 22
| 2 | uint16 | OPC PM bin 23
| 2 | uint16 | OPC PM bin 24
| 1 | uint8 | OPC sample valid
| 8 | int64 | Uptime


Total: 123 bytes

## Enabling FOTA

First commit everything verify the release is read (run `reto status` to see the current status), the build the release:

`touch CMakeFiles.txt && make`

(this ensures a new build with the updated version number)

Upload firmware image, set version number and assign it to the device in Horde:

```shell
curl -s -HX-API-Token:$(cat .apikey) \
    -XPOST -F image=@build/zephyr/zephyr.signed.bin \
    https://api.nbiot.engineering/collections/$(cat .collectionid)/firmware | jq -r ".imageId" > .firmwareid

curl -XPATCH -d'{"version":"0.0.3"}' -HX-API-Token:$(cat .apikey) \
    https://api.nbiot.engineering/collections/$(cat .collectionid)/firmware/$(cat .firmwareid)

FIRMWAREID=$(cat .firmwareid) curl  -HX-API-Token:$(cat .apikey)\
    -XPATCH -d'{"firmware":{"targetFirmwareId": "${FIRMWAREID}"}}' \
    https://api.nbiot.engineering/collections/$(cat .collectionid)/devices/$(cat .deviceid)

```

Prepare the files `.apikey` with the API token, `.collectionid` and `.deviceid` with the collection and device ID from Horde. These can be found and/or created in [The Horde console](https://nbiot.engineering)