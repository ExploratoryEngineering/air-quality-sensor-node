KEYFILE := aq_fota.pem

all:
	west build  --board nrf52_pca10040
	#west sign -t imgtool -- --key $(KEYFILE)

clean:
	rm -fR build

flash:
	west build --board nrf52_pca10040
	#west sign -t imgtool -- --key $(KEYFILE)
	west flash
	#--hex-file build/zephyr/zephyr.signed.hex

# Generate a key. This will generate a NEW KEY for the image. If you have
# deployed with a different key the images will not be recognized by the
# existing firmware.
fwkey:
	imgtool keygen -k $(KEYFILE) -t rsa-2048
