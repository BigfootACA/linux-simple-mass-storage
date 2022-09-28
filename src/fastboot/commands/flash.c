/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"
#include<pthread.h>
#include<sys/mount.h>

static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
struct mon_data{
	char tag[32];
	pthread_t tid;
	volatile bool thread,run;
	size_t size,current;
	uint64_t start,end;
}mon;

static void*disk_io_mon_thread(void*d __attribute__((unused))){
	double pct;
	size_t cnt=0,last,trans;
	char buf1[64],buf2[64],spd[64];
	mon.thread=true,last=mon.current;
	while(mon.run&&mon.size>0){
		sleep(1);
		trans=mon.current-last,last=mon.current;
		if(trans>0&&(cnt%5)==0){
			format_size(spd,sizeof(spd),trans,1,0);
			format_size(buf2,sizeof(buf2),mon.size,1,0);
			format_size(buf1,sizeof(buf1),mon.current,1,0);
			pct=(double)((double)mon.current/(double)mon.size)*100;
			DEBUG(
				"%s %zu/%zu bytes (%s/%s) %s/s %0.2f%%\n",
				mon.tag,mon.current,mon.size,buf1,buf2,spd,pct
			);
		}
		cnt++;
	}
	mon.thread=false;
	mon.tid=0;
	return NULL;
}

static int block_start_trans(const char*path,const char*tag){
	int fd=-1;
	size_t size=0;
	struct stat st;
	if(!path||!path[0]||!tag||!tag[0])ERET(EINVAL);
	if((fd=open(path,O_RDWR))<0)
		EXGOTO(ENOENT,"open block %s failed",path);
	if(fstat(fd,&st)<0)
		EXGOTO(EIO,"stat block %s failed",path);
	if(!S_ISBLK(st.st_mode))EXRET(ENOTBLK);
	if(ioctl(fd,BLKGETSIZE64,&size)<0)
		EXGOTO(EIO,"ioctl block BLKGETSIZE64 %s failed",path);
	if(size<=0)EXRET(ENOMEDIUM);
	pthread_mutex_lock(&lock);
	strncpy(mon.tag,tag,sizeof(mon.tag)-1);
	mon.thread=false,mon.run=true,mon.size=size,mon.current=0;
	pthread_create(&mon.tid,NULL,disk_io_mon_thread,NULL);
	while(!mon.thread);
	mon.start=get_now_ms();
	return fd;
	fail:
	if(fd>=0)close(fd);
	return -1;
}

static int block_end_trans(bool log){
	double time;
	char buf[64],spd[64];
	if(!mon.run)return -1;
	mon.end=get_now_ms();
	time=(float)(mon.end-mon.start)/1000;
	if(time<=0)time=0.01;
	mon.run=false;
	while(mon.thread);
	if(log){
		format_size(buf,sizeof(buf),mon.size,1,0);
		format_size(spd,sizeof(spd),mon.size/time,1,0);
		DEBUG(
			"%s %zu bytes (%s) in %0.2fs speed %s/s\n",
			mon.tag,mon.size,buf,time,spd
		);
	}
	pthread_mutex_unlock(&lock);
	return 0;
}

static int flash_raw_img(int fd,void*img,size_t len){
	char buf[64];
	void*txd=img;
	ssize_t sent;
	size_t txs=len;
	do{
		errno=0;
		sent=write(fd,txd,MIN(txs,16384));
		if(sent<0){
			if(errno==EAGAIN)continue;
			if(errno==EINTR)continue;
			if(errno==0)errno=EIO;
			format_size(buf,sizeof(buf),mon.current,1,0);
			ERROR(
				-1,"write block at %zu bytes (%s) failed",
				mon.current,buf
			);
			EXRET(EIO);
		}
		txs-=sent,txd+=sent,mon.current+=sent;
	}while(txs>0);
	return 0;
}

static int flash_sparse_img_write(ssize_t off,size_t len,void*buff,void*data){
	int fd=*(int*)data;
	if(off>=0&&lseek(fd,(off_t)off,SEEK_SET)<0)
		return ERROR(-1,"seek block to 0x%zx failed",off);
	if(len>0&&buff&&flash_raw_img(fd,buff,len)!=0)
		return ERROR(-1,"write block failed");
	return 0;
}

static int flash_sparse_img(int fd,void*img,size_t len,size_t disk){
	if(sparse_probe(img,len,&mon.size)!=0)return -1;
	if(sparse_parse(
		flash_sparse_img_write,img,
		len,disk,&fd,&mon.current
	)!=0)return -1;
	return 0;
}

static int cmd_flash(fastboot_cmd*cmd,const char*arg){
	errno=0;
	size_t txs;
	int fd=-1,er;
	const char*path;
	void*data=NULL;
	char buf[64],str[256];
	if(!cmd||!arg)return -1;
	if(!(path=fb_get_real_block(arg)))EXRET(EINVAL);
	if(!fb_get_last_download(&data,&txs)){
		ERROR(-1,"host does not sent data");
		EXRET(ENODATA);
	}
	if(!data||txs<=0){
		ERROR(-1,"invalid data");
		ERET(ENODATA);
	}
	if(path[0]!='/')return fb_sends_fail("target not found");
	format_size(buf,sizeof(buf),txs,1,0);
	memset(str,0,sizeof(str));
	snprintf(str,sizeof(str)-1,"request flash %zu bytes (%s) to",txs,buf);
	if(arg[0]=='/')DEBUG("%s '%s'\n",str,path);
	else DEBUG("%s '%s' (%s)\n",str,arg,path);
	if((fd=block_start_trans(path,"write"))<=0)EXRET(EIO);
	if(txs>mon.size)ENGOTO(ENOSPC);
	uint32_t*magic=data;
	if(*magic==SPARSE_HEADER_MAGIC){
		DEBUG("found ANDROID SPARSE image %zu bytes (%s)\n",txs,buf);
		if(flash_sparse_img(fd,data,txs,mon.size)!=0)goto fail;
	}else if(*magic==META_HEADER_MAGIC){
		errno=0;
		ERROR(-1,"META image not supported");
		errno=ENOTSUP;
		goto fail;
	}else{
		mon.size=txs;
		DEBUG("found RAW image %zu bytes (%s)\n",txs,buf);
		if(flash_raw_img(fd,data,txs)!=0)goto fail;
	}
	block_end_trans(true);
	close(fd);
	fastboot_scan_blocks();
	return fb_send_okay();
	fail:
	er=errno;
	block_end_trans(false);
	close(fd);
	fastboot_scan_blocks();
	ERET(er);
}

static int cmd_fetch(fastboot_cmd*cmd,const char*arg){
	ssize_t sent;
	int fd=-1,er;
	const char*path;
	size_t rxs=0,bs=0;
	long long off=0,size=0;
	void*rxd=NULL,*ptr=NULL;
	char*p,*vo,*vs,buf[64],str[256];
	char buff[64],*end=NULL,*name=buff;
	if(!cmd||!arg)return -1;
	memset(buff,0,sizeof(buff));
	strncpy(buff,arg,sizeof(buff));
	if((p=strchr(buff,':'))&&p!=buff){
		*p=0,vo=p+1,name=buff;
		if((p=strchr(vo,':'))&&p!=vo){
			*p=0,vs=p+1,errno=0,size=strtoll(vs,&end,16);
			if(size<=0||end==vs||errno!=0)EXRET(EINVAL);
		}
		errno=0,off=strtoll(vo,&end,16);
		if(off<0||end==vo||errno!=0)EXRET(EINVAL);
	}
	if(!fb_get_buffer(&ptr,&bs))EXRET(ENOMEM);
	if(!(path=fb_get_real_block(name)))EXRET(EINVAL);
	if((fd=block_start_trans(path,"read"))<=0)EXRET(EIO);
	if(size<=0)size=mon.size-off;
	else if((size_t)(off+size)>mon.size)ENGOTO(ERANGE);
	if((size_t)size>bs)ENGOTO(ENOMEM);
	memset(str,0,sizeof(str));
	snprintf(
		str,sizeof(str)-1,
		"request fetch %lld bytes (%s) offset %lld from",
		size,format_size(buf,sizeof(buf),size,1,0),off
	);
	if(name[0]=='/')DEBUG("%s '%s'\n",str,path);
	else DEBUG("%s '%s' (%s)\n",str,name,path);
	errno=0,rxd=ptr,rxs=size,mon.size=size;
	if(off!=0&&lseek(fd,(off_t)off,SEEK_SET)<0)
		EXGOTO(EIO,"seek block %s to %lld failed",name,off);
	do{
		errno=0;
		sent=read(fd,rxd,MIN(rxs,16384));
		if(sent<0){
			if(errno==EAGAIN)continue;
			if(errno==EINTR)continue;
			if(errno==0)errno=EIO;
			format_size(buf,sizeof(buf),mon.current,1,0);
			EXGOTO(
				EIO,"read block at %zu bytes (%s) failed",
				mon.current,buf
			);
		}
		rxs-=sent,rxd+=sent,mon.current=rxd-ptr;
	}while(rxs>0);
	fb_set_last_upload(size);
	block_end_trans(true);
	close(fd);
	if(fb_write_buffer()!=0)EXRET(EIO);
	return fb_send_okay();
	fail:
	er=errno;
	block_end_trans(false);
	close(fd);
	ERET(er);
}

static int cmd_erase(fastboot_cmd*cmd,const char*arg){
	errno=0;
	int fd=-1;
	char buff[4096];
	const char*path;
	if(!cmd||!arg)return -1;
	if(!(path=fb_get_real_block(arg)))EXRET(EINVAL);
	if(path[0]!='/')return fb_sends_fail("target not found");
	if(arg[0]=='/')DEBUG("request erase '%s'\n",path);
	else DEBUG("request erase '%s' (%s)\n",arg,path);
	if((fd=block_start_trans(path,"erase"))<=0)EXRET(EIO);
	memset(buff,0,sizeof(buff));
	while(true)if(write(fd,buff,sizeof(buff))<=0){
		if(errno==ENOSPC)break;
		if(errno==0)errno=EIO;
		ERROR(-1,"erase write failed");
		block_end_trans(false);
		close(fd);
		return fb_send_errno(errno);
	}
	block_end_trans(true);
	close(fd);
	fastboot_scan_blocks();
	return fb_send_okay();
}

int fastboot_constructor_flash(){
	fastboot_register_cmd(
		"flash",
		"Flash to block device",
		"flash:<BLOCK>",
		ARG_REQUIRED,
		cmd_flash
	);
	fastboot_register_cmd(
		"erase",
		"Erase block device",
		"erase:<BLOCK>",
		ARG_REQUIRED,
		cmd_erase
	);
	fastboot_register_cmd(
		"fetch",
		"Read block device",
		"fetch:<BLOCK>[:OFFSET[:SIZE]]",
		ARG_REQUIRED,
		cmd_fetch
	);
	return 0;
}
