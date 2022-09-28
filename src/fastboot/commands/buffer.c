/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fastboot.h"
#include<signal.h>
#include<sys/mman.h>
#include<sys/sysinfo.h>
#include<linux/usb/functionfs.h>

static int buf_type=0;
static void*buf_ptr=NULL;
static size_t buf_size=0;
static size_t last_download=0,last_upload=0;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static struct mon_data{
	pthread_t parent,tid;
	bool timedout;
	volatile bool thread,run;
	size_t size,current;
}mon;

size_t fb_get_buffer_size(){
	if(buf_size==0){
		#if MAX_BUFFER_SIZE == 0
		struct sysinfo sinfo;
		sysinfo(&sinfo);
		size_t ret=sinfo.freeram/2;
		buf_size=MIN(MAX(0x10000000,ret),0xFFFFFFFF);
		#else
		buf_size=(size_t)MAX_BUFFER_SIZE;
		#endif
	}
	return buf_size;
}

void fastboot_free_buffer(){
	if(!buf_ptr)return;
	switch(buf_type){
		case 1:munmap(buf_ptr,buf_size);break;
		case 2:free(buf_ptr);break;
	}
	last_download=0,last_upload=0;
	buf_type=0,buf_ptr=NULL,buf_size=0;
}

bool fb_get_buffer(void**buffer,size_t*size){
	char buf[64];
	if(!buf_ptr){
		if(buf_size==0)buf_size=fb_get_buffer_size();
		format_size(buf,sizeof(buf),buf_size,1,0);
		buf_type=1;
		buf_ptr=mmap(
			NULL,buf_size,PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB,-1,0
		);
		if(!buf_ptr||buf_ptr==MAP_FAILED){
			DEBUG("mmap allocate failed: %m\n");
			buf_type=2;
			if(!(buf_ptr=malloc(buf_size))){
				ERROR(-1,"allocate %zu bytes (%s) failed",buf_size,buf);
				last_download=0,last_upload=0,buf_size=0,buf_type=0;
				return false;
			}
		}
		DEBUG("allocated %zu bytes (%s) for buffer\n",buf_size,buf);
		memset(&mon,0,sizeof(mon));
		last_download=0,last_upload=0;
	}
	if(buffer)*buffer=buf_ptr;
	if(size)*size=buf_size;
	return true;
}

static void*usb_trans_mon_thread(void*d __attribute__((unused))){
	double pct;
	size_t cnt=0,idle=0,last,trans;
	char buf1[64],buf2[64],spd[64];
	mon.thread=true,last=mon.current;
	while(mon.run&&mon.size>0){
		sleep(1);
		trans=mon.current-last,last=mon.current;
		idle=trans==0?idle+1:0;
		if(idle>10){
			mon.timedout=true,idle=0;
			DEBUG("no activity in 10 seconsd\n");
			pthread_kill(mon.parent,SIGALRM);
		}
		if(trans>0&&(cnt%5)==0){
			format_size(spd,sizeof(spd),trans,1,0);
			format_size(buf2,sizeof(buf2),mon.size,1,0);
			format_size(buf1,sizeof(buf1),mon.current,1,0);
			pct=(double)((double)mon.current/(double)mon.size)*100;
			DEBUG(
				"transferred %zu/%zu bytes (%s/%s) %s/s %0.2f%%\n",
				mon.current,mon.size,buf1,buf2,spd,pct
			);
		}
		cnt++;
	}
	mon.thread=false;
	mon.tid=0;
	return NULL;
}

static void empty_handler(int i __attribute__((unused))){}

void*fb_read_buffer(size_t size){
	void*rxd;
	double time;
	ssize_t sent;
	size_t rxs=size;
	uint64_t start,end;
	char buf[64],spd[64];
	if(!buf_ptr)fb_get_buffer(NULL,NULL);
	if(!buf_ptr)EPRET(ENOMEM);
	if(size>buf_size)EPRET(EFBIG);
	pthread_mutex_lock(&lock);
	mon.parent=pthread_self(),mon.run=true;
	mon.size=size,mon.current=0,rxd=buf_ptr;
	mon.thread=false,mon.timedout=false;
	signal(SIGALRM,empty_handler);
	pthread_create(&mon.tid,NULL,usb_trans_mon_thread,NULL);
	while(!mon.thread);
	fb_send_data(size);
	start=get_now_ms();
	do{
		errno=0;
		sent=read(usb_fd_in,rxd,MIN(rxs,16384));
		if(sent<0){
			if(mon.timedout)errno=ETIMEDOUT;
			if(errno==EAGAIN)continue;
			if(errno==EINTR)continue;
			if(errno==0)errno=EIO;
			format_size(buf,sizeof(buf),mon.current,1,0);
			ERROR(-1,"read at %zu bytes (%s) failed",mon.current,buf);
			mon.run=false;
			signal(SIGALRM,SIG_IGN);
			while(mon.thread);
			pthread_mutex_unlock(&lock);
			return NULL;
		}
		rxs-=sent,rxd+=sent,mon.current=rxd-buf_ptr;
	}while(rxs>0);
	end=get_now_ms();
	time=(float)(end-start)/1000;
	if(time<=0)time=0.01;
	last_download=size,last_upload=0,mon.run=false;
	signal(SIGALRM,SIG_IGN);
	while(mon.thread);
	DEBUG(
		"read %zu bytes (%s) in %0.2fs speed %s/s from host\n",
		size,format_size(buf,sizeof(buf),size,1,0),
		time,format_size(spd,sizeof(spd),size/time,1,0)
	);
	pthread_mutex_unlock(&lock);
	return buf_ptr;
}

int fb_write_buffer(){
	void*txd;
	float time;
	ssize_t sent;
	uint64_t start,end;
	char buf[64],spd[64];
	size_t txs=last_upload;
	if(buf_size<=0||!buf_ptr||last_upload<=0)ERET(ENODATA);
	pthread_mutex_lock(&lock);
	mon.parent=pthread_self(),mon.run=true;
	mon.size=last_upload,mon.current=0,txd=buf_ptr;
	mon.thread=false,mon.timedout=false;
	signal(SIGALRM,empty_handler);
	pthread_create(&mon.tid,NULL,usb_trans_mon_thread,NULL);
	while(!mon.thread);
	fb_send_data(last_upload);
	start=get_now_ms();
	do{
		errno=0;
		sent=write(usb_fd_out,txd,MIN(txs,16384));
		if(sent<0){
			if(mon.timedout)errno=ETIMEDOUT;
			if(errno==EAGAIN)continue;
			if(errno==EINTR)continue;
			if(errno==0)errno=EIO;
			format_size(buf,sizeof(buf),mon.current,1,0);
			ERROR(-1,"write at %zu bytes (%s) failed",mon.current,buf);
			mon.run=false;
			signal(SIGALRM,SIG_IGN);
			while(mon.thread);
			pthread_mutex_unlock(&lock);
			return -1;
		}
		txs-=sent,txd+=sent,mon.current=txd-buf_ptr;
	}while(txs>0);
	end=get_now_ms();
	time=(float)(end-start)/1000;
	if(time<=0)time=0.01;
	format_size(buf,sizeof(buf),last_upload,1,0);
	mon.run=false;
	signal(SIGALRM,SIG_IGN);
	while(mon.thread);
	DEBUG(
		"write %zu bytes (%s) in %0.2fs speed %s/s to host\n",
		last_upload,format_size(buf,sizeof(buf),last_upload,1,0),
		time,format_size(spd,sizeof(spd),last_upload/time,1,0)
	);
	pthread_mutex_unlock(&lock);
	return 0;
}

void fb_set_last_upload(size_t size){last_upload=size;}
void fb_set_last_download(size_t size){last_download=size;}
bool fb_get_last_download(void**data,size_t*size){
	if(!buf_ptr||last_download<=0){
		errno=ENODATA;
		return false;
	}
	if(data)*data=buf_ptr;
	if(size)*size=last_download;
	return true;
}

int fb_copy_upload(void*data,size_t size){
	if(size<=0){
		last_upload=0;
		last_download=0;
		return 0;
	}
	if(!buf_ptr)fb_get_buffer(NULL,NULL);
	if(!buf_ptr)ERET(ENOMEM);
	if(size>buf_size)ERET(EFBIG);
	if(!data)ERET(EINVAL);
	memcpy(buf_ptr,data,size);
	return 0;
}

static int cmd_download(fastboot_cmd*cmd,const char*arg){
	char*end=NULL;
	ssize_t len=0;
	if(!cmd||!arg)return -1;
	errno=0,len=strtol(arg,&end,16);
	if(len<=0||end==arg||errno!=0)EXRET(EINVAL);
	if(!fb_read_buffer((size_t)len))EXRET(EIO);
	return fb_send_okay();
}

static int cmd_upload(fastboot_cmd*cmd,const char*arg){
	if(!cmd||arg)EXRET(EINVAL);
	if(fb_write_buffer()!=0)EXRET(EIO);
	return fb_send_okay();
}

static void fastboot_init_buffer();
static int update_var(
	fastboot_var_hook*h __attribute__((unused)),
	fastboot_var_value*old __attribute__((unused)),
	fastboot_var_value*new
){
	if(new->type!=VAR_TYPE_DEC_INT)ERET(EINVAL);
	fastboot_free_buffer();
	buf_size=new->integer;
	fastboot_init_buffer();
	return 0;
}

static void fastboot_init_buffer(){
	size_t size=fb_get_buffer_size();
	fb_set_var_number("max-fetch-size",size);
	fb_set_var_number("max-download-size",size);
	fb_var_add_hook("max-fetch-size",update_var,NULL);
	fb_var_add_hook("max-download-size",update_var,NULL);
}

static int cmd_oem_set_buffer_size(fastboot_cmd*cmd,const char*arg){
	char*end=NULL;
	ssize_t len=0;
	if(!cmd||!arg)EXRET(EINVAL);
	fastboot_free_buffer();
	errno=0,len=strtol(arg,&end,0);
	if(len<=0||end==arg||errno!=0)EXRET(EINVAL);
	buf_size=len;
	fastboot_init_buffer();
	return fb_send_okay();
}

int fastboot_constructor_buffer(){
	fastboot_init_buffer();
	fastboot_register_cmd(
		"upload",
		"Send data to host",
		NULL,ARG_NONE,
		cmd_upload
	);
	fastboot_register_cmd(
		"download",
		"Receive data from host",
		"download:<SIZE>",
		ARG_REQUIRED,
		cmd_download
	);
	fastboot_register_oem_cmd(
		"set-buffer-size",
		"Set upload download buffer size",
		"set-buffer-size <SIZE>",
		ARG_REQUIRED,
		cmd_oem_set_buffer_size
	);
	return 0;
}
