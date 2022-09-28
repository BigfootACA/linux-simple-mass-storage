/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"

static int cmd_getvar(fastboot_cmd*cmd,const char*arg){
	list*l;
	char buff[64];
	fastboot_var*v;
	if(!cmd||!arg)return -1;
	if(strcasecmp(arg,"all")==0){
		if((l=fb_get_vars()))do{
			v=LIST_DATA(l,fastboot_var*);
			if(!v->key[0])continue;
			fb_var_to_str(v,buff,sizeof(buff));
			fb_sendf_info("%s:%s",v->key,buff);
		}while((l=l->next));
		fb_send_okay();
	}else if((v=fb_get_var(arg))){
		fb_var_to_str(v,buff,sizeof(buff));
		fb_sends_okay(buff);
	}else fb_sends_fail("variable not found");
	return 0;
}

static int cmd_oem_setvar(fastboot_cmd*cmd,const char*arg){
	size_t l;
	fastboot_var*v;
	fastboot_var_value nv;
	char*p,*st,*sk,*end=NULL;
	char data[64],key[64],val[64];
	if(!cmd||!arg)return -1;
	memset(&nv,0,sizeof(nv));
	memset(data,0,sizeof(data));
	strncpy(data,arg,sizeof(data)-1);
	memset(key,0,sizeof(key));
	memset(val,0,sizeof(val));
	if(!(p=strchr(data,' '))||p==data)ERET(EINVAL);
	*p=0,sk=p+1,st=data,l=strlen(st);
	if(l<=0)ERET(EINVAL);
	if((p=strchr(sk,' '))&&p!=sk){
		*p++=0;
		strncpy(key,sk,sizeof(key)-1);
		strncpy(val,p,sizeof(val)-1);
	}else strncpy(key,data,sizeof(key)-1);
	if((v=fb_get_var(key))){
		if(!v->deletable&&!val[0])ERET(EPERM);
		if(v->protect)ERET(EPERM);
	}
	if(!val[0]){
		if(!v)ERET(ENOENT);
		return fb_del_var(v);
	}
	errno=0;
	if(strncasecmp(st,"string",l)==0){
		nv.type=VAR_TYPE_STRING;
		strncpy(nv.string,val,sizeof(nv.string)-1);
	}else if(strncasecmp(st,"decimal",l)==0){
		nv.type=VAR_TYPE_DEC_INT;
		nv.integer=(int64_t)strtoll(val,&end,10);
		if(end==val||errno!=0)EXRET(EINVAL);
	}else if(strncasecmp(st,"integer",l)==0){
		nv.type=VAR_TYPE_DEC_INT;
		nv.integer=(int64_t)strtoll(val,&end,0);
		if(end==val||errno!=0)EXRET(EINVAL);
	}else if(strncasecmp(st,"hexadecimal",l)==0){
		nv.type=VAR_TYPE_HEX_INT;
		nv.integer=(int64_t)strtoll(val,&end,16);
		if(end==val||errno!=0)EXRET(EINVAL);
	}else if(strncasecmp(st,"boolean",l)==0){
		nv.type=VAR_TYPE_BOOLEAN;
		if(strcasecmp(val,"yes")==0)nv.boolean=true;
		else if(strcasecmp(val,"no")==0)nv.boolean=false;
		else ERET(EINVAL);
	}else ERET(EINVAL);
	if(fb_set_var(key,&nv)==0)fb_send_okay();
	else fb_send_errno(errno);
	return 0;
}

int fastboot_constructor_variable(){
	fastboot_register_cmd(
		"getvar",
		"Get variable",
		"getvar:<NAME>",
		ARG_REQUIRED,
		cmd_getvar
	);
	fastboot_register_oem_cmd(
		"setvar",
		"Set variable",
		"setvar {str|int|hex|bool} <KEY> [VALUE...]",
		ARG_REQUIRED,
		cmd_oem_setvar
	);
	return 0;
}
