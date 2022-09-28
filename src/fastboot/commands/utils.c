/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"
#include<sys/utsname.h>
#include<sys/klog.h>
#define SYSLOG_ACTION_READ_ALL      3
#define SYSLOG_ACTION_SIZE_BUFFER   10

static int cmd_oem_uname(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	struct utsname uts;
	if(uname(&uts)!=0)
		return fb_send_errno(errno);
	return fb_sends_okay(uts.release);
}

static int cmd_oem_sync(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	sync();
	return fb_send_okay();
}

static int cmd_oem_panic(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	write_file(AT_FDCWD,"c\n",PATH_PROC"/sysrq-trigger");
	return fb_send_fail();
}

static int cmd_oem_unlink(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg
){
	if(unlink(arg)!=0)
		return fb_send_errno(errno);
	return fb_send_okay();
}

static int cmd_oem_rmdir(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg
){
	if(rmdir(arg)!=0)
		return fb_send_errno(errno);
	return fb_send_okay();
}

static int cmd_oem_mkdir(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg
){
	if(mkdir(arg,0755)!=0)
		return fb_send_errno(errno);
	return fb_send_okay();
}

static int cmd_oem_run(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg
){
	pid_t p=fork();
	if(p==0){
		for(int i=3;i<1024;i++)close(i);
		execl(arg,arg,NULL);
		_exit(0);
		return 0;
	}else if(p<0)return fb_send_errno(errno);
	return fb_send_okay();
}

static int cmd_oem_sophon(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	fb_sends_info("OH MY KAWAII SOPHON");
	return fb_send_okay();
}

static int cmd_oem_dmesg(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	size_t size=0;
	int rs=0,len=0;
	void*buff=NULL;
	errno=0;
	if(!fb_get_buffer(&buff,&size))EXRET(ENOMEM);
	errno=0;
	len=klogctl(SYSLOG_ACTION_SIZE_BUFFER,NULL,0);
	if(len<=0)EXRET(ENODATA);
	if((size_t)len>size)EXRET(ENOMEM);
	rs=klogctl(SYSLOG_ACTION_READ_ALL,buff,len);
	if(rs<=0)EXRET(ENODATA);
	if((size_t)rs>=size)ERET(ERANGE);
	fb_set_last_upload(rs);
	fb_sends_info("use fastboot get_staged <OUT> to download your data");
	return fb_send_okay();
}

int fastboot_constructor_utils(){
	fastboot_register_oem_cmd(
		"uname",
		"Get kernel release",
		NULL,ARG_NONE,
		cmd_oem_uname
	);
	fastboot_register_oem_cmd(
		"sync",
		"Sync filesystem",
		NULL,ARG_NONE,
		cmd_oem_sync
	);
	fastboot_register_oem_cmd(
		"panic",
		"Trigger panic",
		NULL,ARG_NONE,
		cmd_oem_panic
	);
	fastboot_register_oem_cmd(
		"sophon",NULL,NULL,ARG_OPTIONAL,
		cmd_oem_sophon
	);
	fastboot_register_oem_cmd(
		"unlink",
		"Delete a file",
		"unlink <FILE>",
		ARG_REQUIRED,
		cmd_oem_unlink
	);
	fastboot_register_oem_cmd(
		"rmdir",
		"Delete a folder",
		"rmdir <FOLDER>",
		ARG_REQUIRED,
		cmd_oem_rmdir
	);
	fastboot_register_oem_cmd(
		"mkdir",
		"Create a folder",
		"mkdir <FOLDER>",
		ARG_REQUIRED,
		cmd_oem_mkdir
	);
	fastboot_register_oem_cmd(
		"run",
		"Execute a program",
		"run <EXECUTE>",
		ARG_REQUIRED,
		cmd_oem_run
	);
	fastboot_register_oem_cmd(
		"dmesg",
		"Read kernel log",
		NULL,ARG_NONE,
		cmd_oem_dmesg
	);
	return 0;
}
