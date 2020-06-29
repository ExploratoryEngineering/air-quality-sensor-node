# Air quality sensor node firmware

This project contains firmware for an air quality sensor node

The board is configured for sampling an Alfasense AFE-3 board populated with NO2, NO and O3 sensors, along with temperature, humidity and an OPC-N3 particulate sensor.

Building:

```sh
west update
source ../deps/zephyr/zephyr-env.sh
west build --pristine -b nrf52_pca10040 Firmware
west flash
```
NOTE!  west v0.7 is required for this to work.

## Required tools

To compile protobuf files you need to install protobuf compiler and python bindingds for protobuf.

For MacOS this can be installed with `brew install protobuf` and `pip install protobuf`.
For Linux this can be installed with `apt-get install protobuf-compiler python-protobuf`.

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

### Flashing and debugging

#### Prerequisites

1. Install [JLink](https://www.segger.com/downloads/jlink/) 
2. The ZEPHYR_BASE environment variable has to point to the deps\zephyr
3. Source deps\zephyr\zephyr-env.sh

#### Flashing

1. The target device has to be powered on
2. Connect the EE-04 programmer to a computer via a USB cable
3. Connect the ribbon debug cable between the EE-04 and the target device
4. `make flash`.

#### Debug output

Open a new shell and issue the following command to connect JLink:

```shell
JLinkExe -if swd -device nrf52 -speed 4000 -autoconnect 1
```

Open a second shell and issue the following command to see log output:

```shell
JLinkRTTClient
```

If you have an existing key put it into the `aq_fota.pem` file and skip this step. This is included in `.gitignore` so you won't check it in by accident.


### Message format

The message format uses protobuffers.  You can find these under
**common/protobuf** in this Github repository.  Both the server and
the firmware uses the same protobuffer definition file.

## Enabling FOTA

First commit everything verify the release is read (run `reto status`
to see the current status), the build the release:

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

Prepare the files `.apikey` with the API token, `.collectionid` and
`.deviceid` with the collection and device ID from Horde. These can be
found and/or created in [The Horde console](https://nbiot.engineering)
