/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fastboot.h"
#include<sys/utsname.h>
#include<sys/sysinfo.h>

static list*vars=NULL;
list*fb_get_vars(){return list_first(vars);}

static bool var_key_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,fastboot_var*);
	return x&&d&&strcasecmp(x->key,d)==0;
}

static bool var_hook_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,fastboot_var_hook*);
	return x&&d&&x->hook==d;
}

fastboot_var*fb_get_var(const char*key){
	if(!key||!key[0])EPRET(EINVAL);
	list*v=list_search_one(
		vars,var_key_cmp,
		(void*)key
	);
	if(!v)EPRET(ENOENT);
	return LIST_DATA(v,fastboot_var*);
}

fastboot_var_value*fb_var_value(fastboot_var*var,fastboot_var_value*val){
	if(!var||!val)EPRET(EINVAL);
	if(var->value.use_ref){
		if(!var->value.ref)EPRET(EINVAL);
		memset(val,0,sizeof(fastboot_var_value));
		val->type=var->value.type;
		switch(var->value.type){
			case VAR_TYPE_STRING:
				strncpy(
					val->string,var->value.ref_string,
					sizeof(val->string)-1
				);
			break;
			case VAR_TYPE_HEX_INT:
			case VAR_TYPE_DEC_INT:
				if(var->value.var_signed)switch(var->value.length){
					case sizeof(int8_t):val->integer=*var->value.ref_int8;break;
					case sizeof(int16_t):val->integer=*var->value.ref_int16;break;
					case sizeof(int32_t):val->integer=*var->value.ref_int32;break;
					case sizeof(int64_t):val->integer=*var->value.ref_int64;break;
					default:EPRET(EINVAL);
				}else switch(var->value.length){
					case sizeof(uint8_t):val->integer=*var->value.ref_uint8;break;
					case sizeof(uint16_t):val->integer=*var->value.ref_uint16;break;
					case sizeof(uint32_t):val->integer=*var->value.ref_uint32;break;
					case sizeof(uint64_t):val->integer=*var->value.ref_uint64;break;
					default:EPRET(EINVAL);
				}
			break;
			case VAR_TYPE_BOOLEAN:val->boolean=*var->value.ref_boolean;break;
			default:EPRET(EINVAL);
		}
	}else if(var->value.type==VAR_TYPE_HANDLER){
		if(!var->value.handler.getter)EPRET(ENOTSUP);
		memset(val,0,sizeof(fastboot_var_value));
		if(var->value.handler.getter(var,val)!=0)return NULL;
	}else memcpy(val,&var->value,sizeof(fastboot_var_value));
	return val;
}

fastboot_var_value*fb_get_var_value(const char*key,fastboot_var_value*val){
	return fb_var_value(fb_get_var(key),val);
}

char*fb_get_var_to_str(const char*key,char*buff,size_t len){
	return fb_var_to_str(fb_get_var(key),buff,len);
}

char*fb_var_to_str(fastboot_var*var,char*buff,size_t len){
	if(!var||!buff||len<=0)EPRET(EINVAL);
	fastboot_var_value v;
	memset(buff,0,len);
	if(!fb_var_value(var,&v))return NULL;
	switch(var->value.type){
		case VAR_TYPE_STRING:strncpy(buff,v.string,len-1);break;
		case VAR_TYPE_BOOLEAN:strncpy(buff,v.boolean?"yes":"no",len-1);break;
		case VAR_TYPE_HEX_INT:snprintf(buff,len-1,"0x%lx",v.integer);break;
		case VAR_TYPE_DEC_INT:snprintf(buff,len-1,"%ld",v.integer);break;
		default:EPRET(EINVAL);
	}
	return buff;
}

const char*fb_get_var_str(const char*key){
	fastboot_var*v=fb_get_var(key);
	if(!v||v->value.type!=VAR_TYPE_STRING)return NULL;
	return v->value.string;
}

int fb_del_var(fastboot_var*t){
	return list_obj_del_data(&vars,t,list_default_free);
}

void fb_clean(){
	list_free_all_def(vars);
	vars=NULL;
}

int fb_del_var_key(const char*key){
	return fb_del_var(fb_get_var(key));
}

int fb_set_var_protected(const char*key,bool protected){
	fastboot_var*v=fb_get_var(key);
	if(!v)return -1;
	v->protect=protected;
	return 0;
}

int fb_set_var_deletable(const char*key,bool deletable){
	fastboot_var*v=fb_get_var(key);
	if(!v)return -1;
	v->deletable=deletable;
	return 0;
}

int fb_var_add_hook(const char*key,fb_var_hook hook,void*data){
	fastboot_var*v;
	fastboot_var_hook h;
	if(!(v=fb_get_var(key)))return -1;
	memset(&h,0,sizeof(h));
	h.var=v,h.hook=hook,h.data=data;
	return list_obj_add_new_dup(&v->hooks,&h,sizeof(h));
}


int fb_var_del_hook(const char*key,fb_var_hook hook){
	list*r;
	fastboot_var*v;
	if(!(v=fb_get_var(key)))return -1;
	if(!(r=list_search_one(v->hooks,var_hook_cmp,hook)))return -1;
	return list_obj_del(&v->hooks,r,list_default_free);
}

int fb_set_var(const char*key,fastboot_var_value*val){
	list*l;
	int r=0;
	fastboot_var*v=NULL;
	if(key&&!val)return fb_del_var_key(key);
	if(!key||!key[0]||!val||!val)ERET(EINVAL);
	if(strcasecmp(key,"all")==0)ERET(EINVAL);
	if((v=fb_get_var(key))){
		if((l=list_first(v->hooks)))do{
			LIST_DATA_DECLARE(h,l,fastboot_var_hook*);
			if(!h||!h->hook)continue;
			if((r=h->hook(h,&v->value,val))!=0)return r;
		}while((l=l->next));
		if(v->value.use_ref){
			switch(val->type){
				case VAR_TYPE_STRING:
					memset(v->value.ref_string,0,v->value.length);
					strncpy(v->value.ref_string,val->string,sizeof(val->string)-1);
				break;
				case VAR_TYPE_HEX_INT:
				case VAR_TYPE_DEC_INT:
					if(v->value.var_signed)switch(v->value.length){
						case sizeof(int8_t):*v->value.ref_int8=val->integer;break;
						case sizeof(int16_t):*v->value.ref_int16=val->integer;break;
						case sizeof(int32_t):*v->value.ref_int32=val->integer;break;
						case sizeof(int64_t):*v->value.ref_int64=val->integer;break;
						default:ERET(EINVAL);
					}else switch(v->value.length){
						case sizeof(uint8_t):*v->value.ref_uint8=val->integer;break;
						case sizeof(uint16_t):*v->value.ref_uint16=val->integer;break;
						case sizeof(uint32_t):*v->value.ref_uint32=val->integer;break;
						case sizeof(uint64_t):*v->value.ref_uint64=val->integer;break;
						default:ERET(EINVAL);
					}
				break;
				case VAR_TYPE_BOOLEAN:*v->value.ref_boolean=val->boolean;break;
				default:ERET(EINVAL);
			}
		}else if(v->value.type==VAR_TYPE_HANDLER&&val->type!=VAR_TYPE_HANDLER){
			if(!v->value.handler.setter)ERET(ENOTSUP);
			if((r=v->value.handler.setter(v,val))!=0)return r;
		}else memcpy(&v->value,val,sizeof(v->value));
		return 0;
	}
	if(!(v=malloc(sizeof(fastboot_var))))ERET(ENOMEM);
	memset(v,0,sizeof(fastboot_var));
	v->deletable=true;
	strncpy(v->key,key,sizeof(v->key)-1);
	memcpy(&v->value,val,sizeof(v->value));
	return list_obj_add_new(&vars,v);
}

int fb_set_var_number(const char*key,const long val){
	fastboot_var_value v={.type=VAR_TYPE_DEC_INT,.integer=val};
	return fb_set_var(key,&v);
}

int fb_set_var_hex(const char*key,const long val){
	fastboot_var_value v={.type=VAR_TYPE_HEX_INT,.integer=val};
	return fb_set_var(key,&v);
}

int fb_set_var_bool(const char*key,const bool val){
	fastboot_var_value v={.type=VAR_TYPE_BOOLEAN,.boolean=val};
	return fb_set_var(key,&v);
}

int fb_set_var_string(const char*key,const char*val){
	fastboot_var_value v;
	memset(&v.string,0,sizeof(v.string));
	strncpy(v.string,val,sizeof(v.string)-1);
	v.type=VAR_TYPE_STRING;
	return fb_set_var(key,&v);
}

int fb_var_mark_all_protected(){
	list*l;
	if((l=fb_get_vars()))do{
		LIST_DATA_DECLARE(v,l,fastboot_var*);
		v->protect=true,v->deletable=false;
	}while((l=l->next));
	return 0;
}

int fb_var_mark_all_undeletable(){
	list*l;
	if((l=fb_get_vars()))do{
		LIST_DATA_DECLARE(v,l,fastboot_var*);
		v->deletable=false;
	}while((l=l->next));
	return 0;
}

void fastboot_init_variables(){
	struct utsname uts;
	struct sysinfo sinfo;
	uname(&uts);
	sysinfo(&sinfo);
	fb_clean();
	fb_set_var_string("version","0.4");
	fb_set_var_string("version-bootloader",uts.release);
	fb_set_var_string("version-baseband",VERSION);
	fb_set_var_string("kernel-version",uts.version);
	fb_set_var_string("kernel-release",uts.release);
	fb_set_var_string("kernel-nodename",uts.nodename);
	fb_set_var_string("kernel-machine",uts.machine);
	fb_set_var_string("kernel-sysname",uts.sysname);
	fb_set_var_string("kernel-sysname",uts.sysname);
	fb_set_var_string("kernel","linux");
	fb_set_var_string("product","Mass Storage");
	fb_set_var_string("variant","Linux Simple Mass Storage");
	fb_set_var_string("serialno",gadget_config.serialnumber);
	fb_set_var_string("build-time",__DATE__" "__TIME__);
	fb_set_var_string("build-compiler",__VERSION__);
	fb_set_var_number("memory",sinfo.totalram);
	fb_set_var_number("swap",sinfo.totalswap);
	fb_set_var_bool("secure",false);
	fb_set_var_bool("is-userspace",true);
	fb_var_mark_all_protected();
}
