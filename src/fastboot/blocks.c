/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fastboot.h"
#include<limits.h>
#include<linux/fs.h>

static list*blocks_map=NULL;
struct block_map{
	bool by_scan;
	char map_name[PATH_MAX];
	char map_path[PATH_MAX];
};

static bool map_name_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,struct block_map*);
	return x&&d&&strcmp(x->map_name,d)==0;
}

struct block_map*map_get_name(const char*key){
	if(!key||!key[0])EPRET(EINVAL);
	list*v=list_search_one(
		blocks_map,
		map_name_cmp,
		(void*)key
	);
	if(!v)EPRET(ENOENT);
	return LIST_DATA(v,struct block_map*);
}

static int map_add(
	const char*name,
	const char*path,
	bool by_scan,
	bool override
){
	struct block_map*v=NULL;
	if(!name||!name[0]||name[0]=='/')ERET(EINVAL);
	if(!path||!path[0]||path[0]!='/')ERET(EINVAL);
	if(access(path,F_OK)!=0)EXRET(ENOENT);
	if((v=map_get_name(name))){
		if(!override)ERET(EEXIST);
		v->by_scan=by_scan;
		memset(v->map_path,0,sizeof(v->map_path));
		strncpy(v->map_path,path,sizeof(v->map_path)-1);
		return 0;
	}
	if(!(v=malloc(sizeof(struct block_map))))ERET(ENOMEM);
	memset(v,0,sizeof(struct block_map));
	v->by_scan=by_scan;
	strncpy(v->map_name,name,sizeof(v->map_name)-1);
	strncpy(v->map_path,path,sizeof(v->map_path)-1);
	list_obj_add_new(&blocks_map,v);
	return 0;
}

static int cmd_oem_maps(
	fastboot_cmd*cmd __attribute__((unused)),
	const char*arg __attribute__((unused))
){
	list*l=NULL;
	char buff[64];
	size_t max_len=2,len,nl,pl;
	if((l=list_first(blocks_map)))do{
		LIST_DATA_DECLARE(map,l,struct block_map*);
		if(!map->map_name[0]||!map->map_path[0])continue;
		len=strlen(map->map_name),max_len=MAX(max_len,len+2);
	}while((l=l->next));
	if((l=list_first(blocks_map)))do{
		LIST_DATA_DECLARE(map,l,struct block_map*);
		if(!map->map_name[0]||!map->map_path[0])continue;
		memset(buff,0,sizeof(buff));
		nl=strlen(map->map_name),pl=strlen(map->map_path);
		len=MAX(MIN(max_len-nl,58),2);
		while(nl+pl+len>=58&&len>1)len--;
		s_strlcat(buff,map->map_name,sizeof(buff));
		for(size_t i=0;i<len;i++)
			s_strlcat(buff," ",sizeof(buff));
		s_strlcat(buff,map->map_path,sizeof(buff));
		fb_sends_info(buff);
	}while((l=l->next));
	return fb_send_okay();
}

static int cmd_oem_setmap(fastboot_cmd*cmd,const char*arg){
	char*p,name[64],path[64];
	if(!cmd||!arg)return -1;
	memset(name,0,sizeof(name));
	memset(path,0,sizeof(path));
	if((p=strchr(arg,' '))&&p!=arg){
		strncpy(name,arg,MIN((size_t)(p-arg),sizeof(name)));
		strncpy(path,p+1,sizeof(path));
	}else strncpy(name,arg,sizeof(name));
	errno=0;
	if(map_add(name,path,false,true)==0){
		fb_send_okay();
		DEBUG("add block map '%s' -> '%s'\n",name,path);
	}else fb_send_errno(errno);
	return 0;
}

const char*fb_get_real_block(const char*name){
	struct block_map*v=NULL;
	if(!name)EPRET(EINVAL);
	if(name[0]=='/')return name;
	if((v=map_get_name(name)))return v->map_path;
	return name;
}

void fastboot_scan_blocks(){
	int fd;
	list*l,*n;
	blkid_dev dev;
	blkid_dev_iterate di;
	blkid_tag_iterate ti;
	size_t size=0,sect=0,cs=0;
	char buff[64],pn[PATH_MAX];
	const char*key,*value,*type,*name,*path;
	blkid_probe_all(fb_blkid_cache);
	di=blkid_dev_iterate_begin(fb_blkid_cache);
	if((l=list_first(blocks_map)))do{
		n=l->next;
		LIST_DATA_DECLARE(map,l,struct block_map*);
		if(!map->map_name[0]||!map->map_path[0])continue;

	}while((l=n));
	while(blkid_dev_next(di,&dev)==0){
		if(!(dev=blkid_verify(
			fb_blkid_cache,dev
		)))continue;
		path=blkid_dev_devname(dev);
		ti=blkid_tag_iterate_begin(dev);
		key=NULL,value=NULL,type="raw",name=NULL;
		while(blkid_tag_next(ti,&key,&value)==0){
			if(strcmp(key,"PARTLABEL")==0)name=value;
			if(strcmp(key,"TYPE")==0)type=value;
			if(
				strcmp(key,"UUID")==0||
				strcmp(key,"LABEL")==0||
				strcmp(key,"PARTUUID")==0||
				strcmp(key,"PARTLABEL")==0
			){
				memset(pn,0,sizeof(pn));
				snprintf(pn,sizeof(pn)-1,"%s=%s",key,value);
				map_add(pn,path,true,true);
				map_add(value,path,true,false);
			}
		}
		if(strncmp(path,"/dev/",5)==0)
			map_add(path+5,path,true,false);
		if(name){
			memset(buff,0,sizeof(buff));
			snprintf(
				buff,sizeof(buff)-1,
				"partition-type:%s",name
			);
			fb_set_var_string(buff,type);
			fb_set_var_protected(buff,true);
			if((fd=open(path,O_RDONLY))>0){
				ioctl(fd,BLKGETSIZE64,&size);
				ioctl(fd,BLKPBSZGET,&cs);
				sect=MAX(sect,cs);
				close(fd);
				memset(buff,0,sizeof(buff));
				snprintf(
					buff,sizeof(buff)-1,
					"partition-size:%s",name
				);
				fb_set_var_hex(buff,size);
				fb_set_var_protected(buff,true);
				format_size(buff,sizeof(buff),size,1,0);
				DEBUG(
					"device %s partlabel %s size %zu bytes (%s)\n",
					path,name,size,buff
				);
			}else DEBUG("device %s partlabel %s\n",path,name);
		}
		blkid_tag_iterate_end(ti);
	}
	blkid_dev_iterate_end(di);
}

int fastboot_constructor_blocks(){
	fastboot_scan_blocks();
	fastboot_register_oem_cmd(
		"maps",
		"List block maps",
		NULL,ARG_NONE,
		cmd_oem_maps
	);
	fastboot_register_oem_cmd(
		"setmap",
		"Set block map",
		"setmap <NAME> <BLOCK>",
		ARG_REQUIRED,
		cmd_oem_setmap
	);
	return 0;
}
