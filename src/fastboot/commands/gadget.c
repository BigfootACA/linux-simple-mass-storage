/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"

static int cmd_gadget_restart(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	fb_sends_info("device will disconnect in a few times");
	fb_send_okay();
	gadget_stop();
	gadget_start();
	return 0;
}

static int cmd_gadget_reinit(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	fb_sends_info("device will disconnect in a few times");
	fb_send_okay();
	gadget_cleanup(GADGET_NAME);
	gadget_init();
	return 0;
}

static int cmd_add_block(fastboot_cmd*cmd,const char*arg){
	errno=0;
	const char*path;
	if(!cmd||!arg)return -1;
	if(!(path=fb_get_real_block(arg)))EXRET(EINVAL);
	if(path[0]!='/')return fb_sends_fail("target not found");
	if(arg[0]=='/')DEBUG("request add block '%s' to mass storage\n",path);
	else DEBUG("request add block '%s' (%s) to mass storage\n",arg,path);
	fb_sends_info("need reinit gadget to apply settings");
	errno=0;
	if(gadget_block_add_path(path)!=0)EXRET(EIO);
	return fb_send_okay();
}

static int cmd_delete_block(fastboot_cmd*cmd,const char*arg){
	errno=0;
	const char*path;
	if(!cmd||!arg)return -1;
	if(!(path=fb_get_real_block(arg)))EXRET(EINVAL);
	if(path[0]!='/')return fb_sends_fail("target not found");
	if(arg[0]=='/')DEBUG("request delete block '%s' from mass storage\n",path);
	else DEBUG("request delete block '%s' (%s) from mass storage\n",arg,path);
	fb_sends_info("need reinit gadget to apply settings");
	errno=0;
	if(gadget_block_delete_path(path)!=0)EXRET(EBUSY);
	return fb_send_okay();
}

static int cmd_clear_blocks(fastboot_cmd*cmd,const char*arg){
	errno=0;
	if(!cmd||arg)return -1;
	fb_sends_info("need reinit gadget to apply settings");
	list_free_all(gadget_blocks,list_default_free);
	gadget_blocks=NULL;
	return fb_send_okay();
}

int fastboot_constructor_gadget(){
	fb_set_var("usb-manufacturer",&(fastboot_var_value){
		.type=VAR_TYPE_STRING,.use_ref=true,
		.length=sizeof(gadget_config.manufacturer),
		.ref_string=gadget_config.manufacturer
	});
	fb_set_var("usb-product",&(fastboot_var_value){
		.type=VAR_TYPE_STRING,.use_ref=true,
		.length=sizeof(gadget_config.product),
		.ref_string=gadget_config.product
	});
	fb_set_var("usb-serialnumber",&(fastboot_var_value){
		.type=VAR_TYPE_STRING,.use_ref=true,
		.length=sizeof(gadget_config.serialnumber),
		.ref_string=gadget_config.serialnumber
	});
	fb_set_var("usb-vendor-id",&(fastboot_var_value){
		.type=VAR_TYPE_HEX_INT,.use_ref=true,
		.length=sizeof(uint16_t),
		.ref_uint16=&gadget_config.vendor_id
	});
	fb_set_var("usb-product-id",&(fastboot_var_value){
		.type=VAR_TYPE_HEX_INT,.use_ref=true,
		.length=sizeof(uint16_t),
		.ref_uint16=&gadget_config.product_id
	});
	fastboot_register_oem_cmd(
		"gadget-restart",
		"Re-Enable gadget UDC",
		NULL,ARG_NONE,
		cmd_gadget_restart
	);
	fastboot_register_oem_cmd(
		"gadget-reinit",
		"Re-Initialize all gadget",
		NULL,ARG_NONE,
		cmd_gadget_reinit
	);
	fastboot_register_oem_cmd(
		"add-block",
		"Add block to mass storage",
		"add-block <BLOCK>",
		ARG_REQUIRED,
		cmd_add_block
	);
	fastboot_register_oem_cmd(
		"delete-block",
		"Delete block from mass storage",
		"delete-block <BLOCK>",
		ARG_REQUIRED,
		cmd_delete_block
	);
	fastboot_register_oem_cmd(
		"clear-blocks",
		"Remove all blocks from mass storage",
		NULL,ARG_NONE,
		cmd_clear_blocks
	);
	return 0;
}
