/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../config.h"
#include<wchar.h>
#include<stdint.h>
#include<linux/usb/functionfs.h>

static const struct{
	struct usb_functionfs_descs_head_v2 header;
	uint32_t fs_count,hs_count,ss_count,os_count;
	struct{
		struct usb_interface_descriptor interface;
		struct usb_endpoint_descriptor_no_audio sink,source;
	}__attribute__((packed)) fs_descs,hs_descs;
	struct{
		struct usb_interface_descriptor interface;
		struct{
			struct usb_endpoint_descriptor_no_audio desc;
			struct usb_ss_ep_comp_descriptor comp;
		}sink,source;
	}__attribute__((packed)) ss_descs;
	struct{
		struct usb_os_desc_header desc;
		struct usb_ext_compat_desc comp;
	}__attribute__((packed)) os_descs;
	struct{
		struct usb_os_desc_header desc;
		struct{
			struct usb_ext_prop_desc prop;
			uint8_t name[20];
			uint32_t data_len;
			uint8_t data[39];
		}__attribute__((packed)) prop;
	}__attribute__((packed)) os_ext_descs;
}__attribute__((packed)) desc={
        .header={
		.magic  = FUNCTIONFS_DESCRIPTORS_MAGIC_V2,
		.length = sizeof(desc),
		.flags  =
			FUNCTIONFS_HAS_FS_DESC|
			FUNCTIONFS_HAS_HS_DESC|
			FUNCTIONFS_HAS_SS_DESC|
			FUNCTIONFS_HAS_MS_OS_DESC,
	},
	.fs_count = 3,
	.hs_count = 3,
	.ss_count = 5,
	.os_count = 2,
	.fs_descs = {
		.interface = {
			.bLength            = USB_DT_INTERFACE_SIZE,
			.bDescriptorType    = USB_DT_INTERFACE,
			.bNumEndpoints      = 2,
			.bInterfaceClass    = FASTBOOT_CLS,
			.bInterfaceSubClass = FASTBOOT_SUBCLS,
			.bInterfaceProtocol = FASTBOOT_PROTO,
			.iInterface         = 1,
		},
		.sink = {
			.bLength            = sizeof(desc.fs_descs.sink),
			.bDescriptorType    = USB_DT_ENDPOINT,
			.bEndpointAddress   = 1|USB_DIR_IN,
			.bmAttributes       = USB_ENDPOINT_XFER_BULK,
		},
		.source = {
			.bLength            = sizeof(desc.fs_descs.source),
			.bDescriptorType    = USB_DT_ENDPOINT,
			.bEndpointAddress   = 2|USB_DIR_OUT,
			.bmAttributes       = USB_ENDPOINT_XFER_BULK,
		},
	},
	.hs_descs = {
		.interface = {
			.bLength            = USB_DT_INTERFACE_SIZE,
			.bDescriptorType    = USB_DT_INTERFACE,
			.bNumEndpoints      = 2,
			.bInterfaceClass    = FASTBOOT_CLS,
			.bInterfaceSubClass = FASTBOOT_SUBCLS,
			.bInterfaceProtocol = FASTBOOT_PROTO,
			.iInterface         = 1,
		},
		.sink = {
			.bLength            = sizeof(desc.hs_descs.sink),
			.bDescriptorType    = USB_DT_ENDPOINT,
			.bEndpointAddress   = 1|USB_DIR_IN,
			.bmAttributes       = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize     = 512,
		},
		.source = {
			.bLength            = sizeof(desc.hs_descs.source),
			.bDescriptorType    = USB_DT_ENDPOINT,
			.bEndpointAddress   = 2|USB_DIR_OUT,
			.bmAttributes       = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize     = 512,
			.bInterval          = 1,
		},
	},
	.ss_descs = {
		.interface = {
			.bLength            = USB_DT_INTERFACE_SIZE,
			.bDescriptorType    = USB_DT_INTERFACE,
			.bNumEndpoints      = 2,
			.bInterfaceClass    = FASTBOOT_CLS,
			.bInterfaceSubClass = FASTBOOT_SUBCLS,
			.bInterfaceProtocol = FASTBOOT_PROTO,
			.iInterface         = 1,
		},
		.sink = {
			.desc = {
				.bLength            = sizeof(desc.ss_descs.sink.desc),
				.bDescriptorType    = USB_DT_ENDPOINT,
				.bEndpointAddress   = 1|USB_DIR_IN,
				.bmAttributes       = USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize     = 1024,
			},
			.comp = {
				.bLength           = sizeof(desc.ss_descs.sink.comp),
				.bDescriptorType   = USB_DT_SS_ENDPOINT_COMP,
				.bMaxBurst         = 15,
			},
		},
		.source = {
			.desc = {
				.bLength            = sizeof(desc.ss_descs.source.desc),
				.bDescriptorType    = USB_DT_ENDPOINT,
				.bEndpointAddress   = 2|USB_DIR_OUT,
				.bmAttributes       = USB_ENDPOINT_XFER_BULK,
			},
			.comp = {
				.bLength            = sizeof(desc.ss_descs.source.comp),
				.bDescriptorType    = USB_DT_SS_ENDPOINT_COMP,
				.bMaxBurst          = 15,
			},
		}
	},
	.os_descs = {
		.desc = {
			.interface = 1,
			.dwLength = sizeof(desc.os_descs),
			.bcdVersion = 1,
			.wIndex = 4,
			.bCount = 1,
		},
		.comp = {
			.Reserved1 = 1,
			.CompatibleID = "WINUSB",
		},
	},
	.os_ext_descs = {
		.desc = {
			.interface = 0,
			.dwLength = sizeof(desc.os_ext_descs),
			.bcdVersion = 1,
			.wIndex = 5,
			.wCount = 1,
		},
		.prop = {
			.prop = {
				.dwSize = sizeof(desc.os_ext_descs.prop),
				.dwPropertyDataType = 1,
				.wPropertyNameLength = sizeof(desc.os_ext_descs.prop.name)
			},
			.name = {
				'D','e','v','i','c','e',
				'I','n','t','e','r','f','a','c','e',
				'G','U','I','D'
			},
			.data_len = sizeof(desc.os_ext_descs.prop.data),
			.data = {
				'{',
				'F','7','2','F','E','0','D','4','-',
				'C','B','C','B','-',
				'4','0','7','D','-',
				'8','8','1','4','-',
				'9','E','D','6','7','3','D','0','D','D','6','B',
				'}'
			},
		},
	},
};
static const struct{
	struct usb_functionfs_strings_head header;
	struct{
		uint16_t code;
		const char str1[9];
	}__attribute__((packed))lang0;
}__attribute__((packed))strings={
	.header={
		.magic=FUNCTIONFS_STRINGS_MAGIC,
		.length=sizeof(strings),
		.str_count=1,
		.lang_count=1,
	},
	.lang0={0x0409,"fastboot"},
};
