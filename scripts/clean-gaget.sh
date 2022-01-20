#!/bin/bash
set -x
CONFIGFS=/sys/kernel/config
find "$CONFIGFS/target" -type l | sort -r | xargs rm 2>/dev/null
find "$CONFIGFS/target" -type d | sort -r | xargs rmdir 2>/dev/null
find "$CONFIGFS/usb_gadget" -mindepth 1 -maxdepth 1 -type d | while read -r gadget
do	echo > "$gadget/UDC" 2>/dev/null
	find "$gadget/strings" -mindepth 1 -maxdepth 1 -type d -exec rmdir {} \;
	find "$gadget/configs" -mindepth 1 -maxdepth 1 -type d | while read -r config
	do	find "$config" -mindepth 1 -maxdepth 1 -type l -exec rm {} \;
		find "$config/strings" -mindepth 1 -maxdepth 1 -type d -exec rmdir {} \;
		rmdir "$config"
	done
	find "$gadget/functions" -mindepth 1 -maxdepth 1 -type d -exec rmdir {} \;
	rmdir "$gadget"
done
