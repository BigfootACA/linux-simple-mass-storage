/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FASTBOOT_H
#define _FASTBOOT_H
#include"../config.h"
#include"../init.h"
#include"../list.h"
#include<stdbool.h>
#include<pthread.h>
#include<libblkid/blkid.h>
#define META_HEADER_MAGIC   0xCE1AD63C
#define SPARSE_HEADER_MAGIC 0xED26FF3A
typedef struct fastboot_cmd fastboot_cmd;
typedef struct fastboot_var fastboot_var;
typedef struct fastboot_var_hook fastboot_var_hook;
typedef struct fastboot_var_value fastboot_var_value;
typedef enum fastboot_var_type fastboot_var_type;
typedef int(*sparse_write_cb)(ssize_t off,size_t len,void*buffer,void*data);
typedef int(*fb_var_handler)(fastboot_var*var,fastboot_var_value*val);
typedef int(*fb_var_hook)(fastboot_var_hook*hook,fastboot_var_value*old,fastboot_var_value*new);
typedef int(*fb_cmd_hand)(fastboot_cmd*cmd,const char*arg);
typedef int(*fb_constructor)();
struct fastboot_cmd{
	char cmd[24];
	char desc[256];
	char usage[64];
	enum fastboot_cmd_type{
		TYPE_BUILTIN,
		TYPE_OEM,
	}type;
	enum fastboot_cmd_arg{
		ARG_NONE,
		ARG_REQUIRED,
		ARG_OPTIONAL,
	}arg;
	fb_cmd_hand hand;
};
enum fastboot_var_type{
	VAR_TYPE_NONE,
	VAR_TYPE_STRING,
	VAR_TYPE_HEX_INT,
	VAR_TYPE_DEC_INT,
	VAR_TYPE_BOOLEAN,
	VAR_TYPE_HANDLER,
};
struct fastboot_var_value{
	fastboot_var_type type:32;
	bool var_signed,use_ref;
	size_t length;
	union{
		char string[64];
		int64_t integer;
		bool boolean;
		struct{
			fb_var_handler getter;
			fb_var_handler setter;
		}handler;
		void*ref;
		char*ref_string;
		bool*ref_boolean;
		int8_t*ref_int8;
		int16_t*ref_int16;
		int32_t*ref_int32;
		int64_t*ref_int64;
		uint8_t*ref_uint8;
		uint16_t*ref_uint16;
		uint32_t*ref_uint32;
		uint64_t*ref_uint64;
	};
};
struct fastboot_var_hook{
	fastboot_var*var;
	fb_var_hook hook;
	void*data;
};
struct fastboot_var{
	char key[64];
	bool protect,deletable;
	fastboot_var_value value;
	list*hooks;
};

extern fb_constructor fb_constructors[];
extern blkid_cache fb_blkid_cache;
extern int usb_fd_ctl,usb_fd_in,usb_fd_out;
extern int fastboot_hand_cmd(const char*buff);
extern int fastboot_register_cmd(
	const char*cmd,
	const char*desc,
	const char*usage,
	enum fastboot_cmd_arg arg,
	fb_cmd_hand hand
);
extern int fastboot_register_oem_cmd(
	const char*cmd,
	const char*desc,
	const char*usage,
	enum fastboot_cmd_arg arg,
	fb_cmd_hand hand
);
extern int sparse_probe(void*img,size_t len,size_t*real_size);
extern int sparse_parse(sparse_write_cb cb,void*img,size_t img_len,size_t disk_size,void*data,size_t*progress);
extern void fastboot_init_variables();
extern void fastboot_scan_blocks();
extern void fastboot_free_buffer();
extern bool fb_get_last_download(void**data,size_t*size);
extern list*fb_get_vars();
extern fastboot_var*fb_get_var(const char*key);
extern const char*fb_get_real_block(const char*name);
extern const char*fb_get_var_val(const char*key);
extern void fb_clean();
extern size_t fb_get_buffer_size();
extern bool fb_get_buffer(void**buffer,size_t*size);
extern void*fb_read_buffer(size_t size);
extern int fb_write_buffer();
extern int fb_copy_upload(void*data,size_t size);
extern void fb_set_last_upload(size_t size);
extern void fb_set_last_download(size_t size);
extern bool fb_get_last_download(void**data,size_t*size);
extern char*fb_get_var_to_str(const char*key,char*buff,size_t len);
extern char*fb_var_to_str(fastboot_var*var,char*buff,size_t len);
extern int fb_del_var(fastboot_var*t);
extern int fb_del_var_key(const char*key);
extern int fb_set_var_protected(const char*key,bool protected);
extern int fb_set_var_deletable(const char*key,bool deletable);
extern int fb_var_add_hook(const char*key,fb_var_hook hook,void*data);
extern int fb_var_del_hook(const char*key,fb_var_hook hook);
extern int fb_set_var(const char*key,fastboot_var_value*val);
extern int fb_set_var_string(const char*key,const char*val);
extern int fb_set_var_hex(const char*key,const long val);
extern int fb_set_var_number(const char*key,const long val);
extern int fb_set_var_bool(const char*key,const bool val);
extern int fb_send_status(const char*name,const char*val);
extern int fb_send_number(const char*name,const int number);
extern int fb_send_data(const int number);
extern int fb_send_okay();
extern int fb_send_info();
extern int fb_send_fail();
extern int fb_sends_okay(const char*data);
extern int fb_sends_info(const char*info);
extern int fb_sends_fail(const char*err);
extern int fb_sendf_okay(const char*data,...);
extern int fb_sendf_info(const char*info,...);
extern int fb_sendf_fail(const char*err,...);
extern int fb_send_errno(int err);
extern int fastboot_init(const char*path);
extern int fastboot_stop();
#endif
