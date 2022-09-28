/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"
#include<syscall.h>
#include<sys/reboot.h>
#include<linux/reboot.h>

static inline int reboot2(int oper,char*data){
	return (int)syscall(
		SYS_reboot,
		LINUX_REBOOT_MAGIC1,
		LINUX_REBOOT_MAGIC2,
		oper,data
	);
}

static int cmd_reboot(fastboot_cmd*cmd,const char*arg){
	int oper=LINUX_REBOOT_CMD_RESTART;
	char*data=(char*)arg;
	if(!cmd)return -1;
	if(strcasecmp(cmd->cmd,"reboot-bootloader")==0)
		data="bootloader";
	else if(strcasecmp(cmd->cmd,"reboot-recovery")==0)
		data="recovery";
	else if(strcasecmp(cmd->cmd,"reboot")!=0)
		data=cmd->cmd;
	if(data){
		if(strcasecmp(data,"halt")==0)oper=LINUX_REBOOT_CMD_HALT;
		else if(strcasecmp(data,"kexec")==0)oper=LINUX_REBOOT_CMD_KEXEC;
		else if(strcasecmp(data,"poweroff")==0)oper=LINUX_REBOOT_CMD_POWER_OFF;
		else if(strcasecmp(data,"powerdown")==0)oper=LINUX_REBOOT_CMD_POWER_OFF;
		else if(strcasecmp(data,"shutdown")==0)oper=LINUX_REBOOT_CMD_POWER_OFF;
		else if(strcasecmp(data,"reboot")==0)oper=LINUX_REBOOT_CMD_RESTART;
		else if(strcasecmp(data,"restart")==0)oper=LINUX_REBOOT_CMD_RESTART;
		else if(strcasecmp(data,"suspend")==0)oper=LINUX_REBOOT_CMD_SW_SUSPEND;
		else oper=LINUX_REBOOT_CMD_RESTART2;
	}
	DEBUG("request reboot command %s\n",data);
	fb_send_okay();
	sync();
	return oper==(int)LINUX_REBOOT_CMD_RESTART2?
		reboot2(oper,data):reboot(oper);
}

int fastboot_constructor_reboot(){
	fastboot_register_cmd(
		"reboot",
		"Reboot",
		NULL,ARG_OPTIONAL,
		cmd_reboot
	);
	fastboot_register_cmd(
		"reboot-bootloader",
		"Reboot into Bootloader",
		NULL,ARG_NONE,
		cmd_reboot
	);
	fastboot_register_cmd(
		"powerdown",
		"Shutdown",
		NULL,ARG_NONE,
		cmd_reboot
	);
	return 0;
}