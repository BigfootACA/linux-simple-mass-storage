/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fastboot.h"
#define DECL_CONS(name)extern int fastboot_constructor_##name();
DECL_CONS(file)
DECL_CONS(flash)
DECL_CONS(utils)
DECL_CONS(blocks)
DECL_CONS(buffer)
DECL_CONS(gadget)
DECL_CONS(reboot)
DECL_CONS(command)
DECL_CONS(variable)
fb_constructor fb_constructors[]={
	fastboot_constructor_file,
	fastboot_constructor_flash,
	fastboot_constructor_utils,
	fastboot_constructor_blocks,
	fastboot_constructor_buffer,
	fastboot_constructor_gadget,
	fastboot_constructor_reboot,
	fastboot_constructor_command,
	fastboot_constructor_variable,
	NULL
};
