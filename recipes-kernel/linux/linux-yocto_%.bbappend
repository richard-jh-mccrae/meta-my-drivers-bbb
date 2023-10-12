FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://0001-led-device-tree.patch "

# prepend to patch file
# arch/arm/boot/dts/
