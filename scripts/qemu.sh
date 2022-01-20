#!/bin/bash
exec qemu-system-aarch64 \
	-nodefaults \
	-machine virt \
	-cpu cortex-a53 \
	-kernel Image.gz \
	-chardev stdio,id=char0 \
	-serial chardev:char0 \
	-bios /usr/share/edk2-armvirt/aarch64/QEMU_CODE.fd \
	-drive file=test.img,format=raw,if=none,id=sda \
	-device pcie-root-port \
	-device nvme,drive=sda,serial=sda \
	-device qemu-xhci \
	-device usb-mouse \
	-device usb-tablet \
	-device usb-kbd \
	-device ramfb \
	-display sdl
