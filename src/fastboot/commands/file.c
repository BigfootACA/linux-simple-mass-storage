/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"
#include<pwd.h>
#include<grp.h>

static int cmd_oem_chown(fastboot_cmd*cmd,const char*arg){
	long uv=-1,gv=-1;
	char*p,buff[64],*path;
	char*vu,*vg,*end=NULL;
	struct group*gr=NULL;
	struct passwd*pw=NULL;
	if(!cmd||!arg)ERET(EINVAL);
	memset(buff,0,sizeof(buff));
	strncpy(buff,arg,sizeof(buff));
	if(!(p=strchr(buff,' '))||p==buff)ERET(EINVAL);
	*p=0,path=p+1,vu=buff;
	if(!(p=strchr(vu,':')))vg=NULL;
	else *p=0,vg=p+1;
	errno=0,uv=strtol(vu,&end,0);
	if(uv<=0||end==vu||errno!=0){
		if(!(pw=getpwnam(vu)))ERET(EINVAL);
		uv=pw->pw_uid;
	}
	if(vg){
		errno=0,gv=strtol(vg,&end,0);
		if(gv<=0||end==vg||errno!=0){
			if(!(gr=getgrnam(vg)))ERET(EINVAL);
			gv=gr->gr_gid;
		}
	}
	if(chown(path,uv,gv)!=0)EXRET(EIO);
	return fb_send_okay();
}

static int cmd_oem_chmod(fastboot_cmd*cmd,const char*arg){
	long mode=0;
	char*p,buff[64],*path,*mod,*end=NULL;
	if(!cmd||!arg)ERET(EINVAL);
	memset(buff,0,sizeof(buff));
	strncpy(buff,arg,sizeof(buff));
	if(!(p=strchr(buff,' '))||p==buff)ERET(EINVAL);
	*p=0,path=p+1,mod=buff,errno=0,mode=strtol(mod,&end,8);
	if(mode<=0||end==mod||errno!=0)EXRET(EINVAL);
	if(chmod(path,mode)!=0)EXRET(EIO);
	return fb_send_okay();
}

static int cmd_oem_write(fastboot_cmd*cmd,const char*arg){
	int fd;
	ssize_t r=0;
	size_t len=0;
	void*buffer=NULL;
	if(!cmd||!arg||arg[0]!='/')ERET(EINVAL);
	if(!fb_get_last_download(&buffer,&len)){
		fb_sends_info(
			"you need use fastboot get_staged <IN>"
			" to upload your data"
		);
		ERET(ENODATA);
	}
	if(!buffer||len<=0)ERET(ENODATA);
	DEBUG("request write file '%s'\n",arg);
	errno=0,fd=open(arg,O_CREAT|O_TRUNC|O_WRONLY,0644);
	if(fd<0)return -1;
	r=full_write(fd,buffer,len);
	close(fd);
	if(r<0)return -1;
	if((size_t)r!=len)ERET(EIO);
	DEBUG("write file '%s' size %zu\n",arg,len);
	return fb_send_okay();
}

static int cmd_oem_read(fastboot_cmd*cmd,const char*arg){
	int fd;
	ssize_t r=0;
	struct stat st;
	size_t len=0,bs=0;
	void*buffer=NULL;
	if(!cmd||!arg||arg[0]!='/')ERET(EINVAL);
	if(!fb_get_buffer(&buffer,&bs))ERET(ENOMEM);
	DEBUG("request read file '%s'\n",arg);
	errno=0,fd=open(arg,O_RDONLY);
	if(fd<0)return -1;
	if(fstat(fd,&st)==0)len=st.st_size;
	if(len>=bs)ENGOTO(EFBIG);
	if(len<=0){
		r=read(fd,buffer,bs);
		if(r>=0)len=r;
	}else r=full_read(fd,buffer,len);
	close(fd);
	if(r<0)return -1;
	if((size_t)r!=len)ERET(EIO);
	fb_set_last_upload(len);
	DEBUG("read file '%s' size %zu\n",arg,len);
	fb_sends_info(
		"use fastboot get_staged <OUT>"
		" to download your data"
	);
	return fb_send_okay();
	fail:
	if(fd>=0)close(fd);
	return -1;
}

static int cmd_oem_ls(fastboot_cmd*cmd,const char*arg){
	DIR*d;
	char buff[BUFSIZ];
	struct dirent*e;
	if(!cmd||!arg||arg[0]!='/')ERET(EINVAL);
	DEBUG("request ls folder '%s'\n",arg);
	errno=0;
	if(!(d=opendir(arg)))return -1;
	while((e=readdir(d))){
		memset(buff,0,sizeof(buff));
		strncpy(buff,e->d_name,sizeof(buff)-1);
		if(e->d_type==DT_DIR)s_strlcat(
			buff,"/",sizeof(buff)-1
		);
		fb_sends_info(buff);
	}
	return fb_send_okay();
}

int fastboot_constructor_file(){
	fastboot_register_oem_cmd(
		"read",
		"Read a file",
		"read <FILE>",
		ARG_REQUIRED,
		cmd_oem_read
	);
	fastboot_register_oem_cmd(
		"write",
		"Write a file",
		"write <FILE>",
		ARG_REQUIRED,
		cmd_oem_write
	);
	fastboot_register_oem_cmd(
		"chown",
		"Change file owner or group",
		"chown <OWNER>[:<GROUP] <FILE|FOLDER>",
		ARG_REQUIRED,
		cmd_oem_chown
	);
	fastboot_register_oem_cmd(
		"chmod",
		"Change file mode or permission",
		"chmod <MODE> <FILE|FOLDER>",
		ARG_REQUIRED,
		cmd_oem_chmod
	);
	fastboot_register_oem_cmd(
		"ls",
		"List items of directory",
		"ls <FOLDER>",
		ARG_REQUIRED,
		cmd_oem_ls
	);
	return 0;
}
