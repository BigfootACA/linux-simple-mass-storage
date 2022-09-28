/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fastboot.h"

static list*commands=NULL;

static int cmd_oem_help(fastboot_cmd*cmd __attribute__((unused)),const char*arg){
	list*l=NULL;
	bool found=false;
	size_t max_len=2,len,cl,dl;
	char buff[64],*prefix="";
	enum fastboot_cmd_type type=TYPE_BUILTIN;
	if(arg){
		if(strncasecmp(arg,"oem ",4)==0)
			type=TYPE_OEM,arg+=4,prefix="oem ";
		if((l=list_first(commands)))do{
			LIST_DATA_DECLARE(cmd,l,fastboot_cmd*);
			DEBUG("%p %d %d '%s' '%s'\n",cmd->hand,type,cmd->type,arg,cmd->cmd);
			if(!cmd->hand||type!=cmd->type)continue;
			if(strcasecmp(arg,cmd->cmd)!=0)continue;
			fb_sends_info("Command:");
			fb_sendf_info("\t%s%s",prefix,cmd->cmd);
			fb_send_info();
			if(cmd->usage[0]){
				fb_sends_info("Usage:");
				fb_sendf_info("\t%s%s",prefix,cmd->usage);
				fb_send_info();
			}
			if(cmd->desc[0]){
				fb_sends_info("Description:");
				fb_sendf_info("\t%s%s",prefix,cmd->desc);
				fb_send_info();
			}
			found=true;
		}while((l=l->next));
		if(!found)return fb_sends_fail("command not found");
	}else{
		if((l=list_first(commands)))do{
			LIST_DATA_DECLARE(cmd,l,fastboot_cmd*);
			if(!cmd->cmd[0]||!cmd->desc[0])continue;
			len=strlen(cmd->cmd);
			if(cmd->type==TYPE_OEM)len+=4;
			max_len=MAX(max_len,len+2);
		}while((l=l->next));
		if((l=list_first(commands)))do{
			LIST_DATA_DECLARE(cmd,l,fastboot_cmd*);
			if(!cmd->cmd[0]||!cmd->desc[0])continue;
			memset(buff,0,sizeof(buff));
			cl=strlen(cmd->cmd),dl=strlen(cmd->desc);
			len=max_len-cl;
			if(cmd->type==TYPE_OEM){
				s_strlcat(buff,"oem ",sizeof(buff));
				len-=4;
			}
			len=MAX(MIN(len,20),2);
			while(cl+dl+len>=58&&len>1)len--;
			s_strlcat(buff,cmd->cmd,sizeof(buff));
			for(size_t i=0;i<len;i++)
				s_strlcat(buff," ",sizeof(buff));
			s_strlcat(buff,cmd->desc,sizeof(buff));
			fb_sends_info(buff);
		}while((l=l->next));
	}
	return fb_send_okay();
}

int fastboot_hand_cmd(const char*buff){
	int r;
	list*l=NULL;
	size_t cl=0;
	fb_constructor c;
	enum fastboot_cmd_type type=TYPE_BUILTIN;
	DEBUG("handle command '%s'\n",buff);
	if(!commands)for(size_t i=0;(c=fb_constructors[i]);i++)c();
	if(strncasecmp(buff,"oem ",4)==0)type=TYPE_OEM,buff+=4;
	if((l=list_first(commands)))do{
		LIST_DATA_DECLARE(cmd,l,fastboot_cmd*);
		if(!cmd->hand||type!=cmd->type)continue;
		if((cl=strlen(cmd->cmd))<=0)continue;
		if(strncasecmp(buff,cmd->cmd,cl)!=0)continue;
		buff+=cl;
		if(cmd->arg==ARG_NONE&&*buff!=0)continue;
		if(cmd->arg==ARG_REQUIRED&&*buff==0)continue;
		if(*buff){
			if(type==TYPE_OEM&&*buff!=' ')continue;
			if(type==TYPE_BUILTIN&&*buff!=':')continue;
			buff++;
		}
		errno=0,r=cmd->hand(cmd,*buff?buff:NULL);
		if(r!=0){
			if(errno>0)fb_send_errno(errno);
			else fb_sends_fail("command failed");
		}
		return r;
	}while((l=l->next));
	return fb_sends_fail("unknown command");
}

int fastboot_constructor_command(){
	fastboot_register_oem_cmd(
		"help",
		"Show help",
		"help [COMMAND]",
		ARG_OPTIONAL,
		cmd_oem_help
	);
	return 0;
}

static int fastboot_register_cmd_internal(
	const char*cmd,
	const char*desc,
	const char*usage,
	enum fastboot_cmd_arg arg,
	enum fastboot_cmd_type type,
	fb_cmd_hand hand
){
	fastboot_cmd c;
	if(!cmd)ERET(EINVAL);
	memset(&c,0,sizeof(c));
	strncpy(c.cmd,cmd,sizeof(c.cmd));
	if(desc)strncpy(c.desc,desc,sizeof(c.desc));
	if(usage)strncpy(c.usage,usage,sizeof(c.usage));
	c.arg=arg,c.type=type,c.hand=hand;
	return list_obj_add_new_dup(&commands,&c,sizeof(c));
}

int fastboot_register_cmd(
	const char*cmd,
	const char*desc,
	const char*usage,
	enum fastboot_cmd_arg arg,
	fb_cmd_hand hand
){
	return fastboot_register_cmd_internal(
		cmd,desc,usage,arg,TYPE_BUILTIN,hand
	);
}

int fastboot_register_oem_cmd(
	const char*cmd,
	const char*desc,
	const char*usage,
	enum fastboot_cmd_arg arg,
	fb_cmd_hand hand
){
	return fastboot_register_cmd_internal(
		cmd,desc,usage,arg,TYPE_OEM,hand
	);
}
