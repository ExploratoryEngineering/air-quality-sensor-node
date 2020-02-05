KEYFILE := aq_fota.pem

all:
	west build  --board nrf52_pca10040
	west sign -t imgtool -- --key $(KEYFILE)

clean:
	rm -fR build

flash:
	west build --board nrf52_pca10040
	west sign -t imgtool -- --key $(KEYFILE)
	west flash --hex-file build/zephyr/zephyr.signed.hex

# Generate a key. This will generate a NEW KEY for the image. If you have
# deployed with a different key the images will not be recognized by the
# existing firmware.
fwkey:
	imgtool keygen -k $(KEYFILE) -t rsa-2048


build_mcuboot:
	git clone https://github.com/JuulLabs-OSS/mcuboot
	cd mcuboot/boot/zephyr
	git checkout v1.4.0
	sed -i -e 's+root-rsa-2048.pem+../aq_fota.pem+g' prj.conf

install_bootloader:
	cd mcuboot/boot/zephyr
	west build
	west flash
