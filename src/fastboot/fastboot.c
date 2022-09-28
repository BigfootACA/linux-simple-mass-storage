/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../config.h"
#include"../init.h"
#include"fastboot.h"
#include<endian.h>
#include<pthread.h>
#include<pthread.h>
#include<stdarg.h>
#include<sys/select.h>
#include<linux/usb/gadgetfs.h>
#include<linux/usb/functionfs.h>
#include"descriptors.h"

blkid_cache fb_blkid_cache=NULL;
int usb_fd_ctl=-1,usb_fd_in=-1,usb_fd_out=-1;
static struct fastboot_stat{
	bool run;
	volatile bool io_stopped;
	volatile bool ctl_stopped;
	pthread_t io_tid;
	pthread_t ctrl_tid;
}st;

int fb_send_status(const char*name,const char*val){
	char buff[64];
	snprintf(
		buff,sizeof(buff)-1,"%s%s",
		name?name:"",val?val:""
	);
	print(usb_fd_out,buff);
	return 0;
}

int fb_send_number(const char*name,const int number){
	char buff[32];
	snprintf(buff,sizeof(buff)-1,"%08x",number);
	return fb_send_status(name,buff);
}

int fb_send_errno(int err){return fb_sends_fail(strerror(err));}
int fb_send_data(const int number){return fb_send_number("DATA",number);}

#define DECL_SEND_STR(name,resp)\
	int fb_send_##name(){return fb_send_status(#resp,NULL);}\
	int fb_sends_##name(const char*data){return fb_send_status(#resp,data);}\
	int fb_sendf_##name(const char*data,...){\
		va_list va;\
		char buff[64];\
		memset(buff,0,sizeof(buff));\
		va_start(va,data);\
		vsnprintf(buff,sizeof(buff)-1,data,va);\
		va_end(va);\
		return fb_send_status(#resp,buff);\
	}\

DECL_SEND_STR(okay,OKAY)
DECL_SEND_STR(fail,FAIL)
DECL_SEND_STR(info,INFO)

static void close_functionfs(){
	st.run=false;
	if(usb_fd_in>=0)close(usb_fd_in);
	if(usb_fd_out>=0)close(usb_fd_out);
	if(usb_fd_ctl>=0)close(usb_fd_ctl);
	usb_fd_in=-1,usb_fd_out=-1,usb_fd_ctl=-1;
}

static int deinit_functionfs(){
	if(usb_fd_in<0||usb_fd_out<0||usb_fd_ctl<0)ERET(EINVAL);
	DEBUG("shutdown fastboot...\n");
	st.run=false;
	ioctl(usb_fd_in,FUNCTIONFS_CLEAR_HALT);
	ioctl(usb_fd_out,FUNCTIONFS_CLEAR_HALT);
	close_functionfs();
	return 0;
}

static void*fastboot_io_thread(void*data __attribute__((unused))){
	int e=0;
	ssize_t r=0;
	char*buff=NULL;
	size_t packet_size=8192;
	struct usb_endpoint_descriptor dsc;
	if(usb_fd_in<0||usb_fd_out<0)goto end;
	DEBUG("usb io thread started\n");
	memset(&dsc,0,sizeof(dsc));
	if(ioctl(usb_fd_in,FUNCTIONFS_ENDPOINT_DESC,&dsc)==0){
		packet_size=dsc.wMaxPacketSize;
		DEBUG("packet size for read buffer: %zu\n",packet_size);
	}else ERROR(-1,"get in endpoint desc failed");
	if(!(buff=malloc(packet_size)))
		ERGOTO(ENOMEM,"alloc read buffer failed");
	st.io_stopped=false;
	while(st.run&&e==0){
		memset(buff,0,packet_size);
		r=read(usb_fd_in,buff,packet_size);
		if(r<0){
			if(errno==EINTR)continue;
			if(errno==EAGAIN)continue;
			if(errno==ESHUTDOWN)goto end;
			EGOTO(-1,"usb bulk-in read failed");
		}
		if(r==0)EGOTO(-1,"usb bulk-in read got EOF");
		fastboot_hand_cmd(buff);
	}
	fail:
	deinit_functionfs();
	end:st.io_tid=0;
	st.io_stopped=true;
	if(buff)free(buff);
	return NULL;
}

static void start_io_thread(){
	if(st.io_tid!=0)return;
	st.io_stopped=true;
	pthread_create(&st.io_tid,NULL,fastboot_io_thread,NULL);
}

static void*fastboot_ctrl_thread(void*data __attribute__((unused))){
	int e=0;
	fd_set fds;
	ssize_t r=0;
	struct timeval t;
	struct usb_functionfs_event evs[5];
	if(usb_fd_ctl<0)goto end;
	DEBUG("usb control thread started\n");
	st.ctl_stopped=false;
	while(st.run&&e==0){
		FD_ZERO(&fds);
		FD_SET(usb_fd_ctl,&fds);
		t.tv_sec=1,t.tv_usec=0;
		if(select(FD_SETSIZE,&fds,NULL,NULL,&t)<0)switch(errno){
			case EAGAIN:case EINTR:continue;
			default:EGOTO(-1,"usb poll failed");
		}
		if(FD_ISSET(usb_fd_ctl,&fds)){
			r=read(usb_fd_ctl,&evs,sizeof(evs));
			if(r<0){
				if(errno==EAGAIN)continue;
				if(errno==EINTR)continue;
				EGOTO(-1,"usb control read failed");
				break;
			}
			for(size_t i=0;i<(size_t)r/sizeof(evs[0]);i++)switch(evs[i].type){
				case FUNCTIONFS_BIND:
					start_io_thread();
					DEBUG("usb receive bind event\n");
				break;
				case FUNCTIONFS_UNBIND:
					DEBUG("usb receive unbind event\n");
				break;
				case FUNCTIONFS_ENABLE:
					start_io_thread();
					DEBUG("usb receive enable event\n");
				break;
				case FUNCTIONFS_DISABLE:
					DEBUG("usb receive disable event\n");
				break;
				case FUNCTIONFS_SETUP:
					DEBUG("usb receive setup event\n");
				break;
				case FUNCTIONFS_SUSPEND:
					DEBUG("usb receive suspend event\n");
				break;
				case FUNCTIONFS_RESUME:
					start_io_thread();
					DEBUG("usb receive resume event\n");
				break;
				default:DEBUG("unknown usb control event %d\n",evs[i].type);break;
			}
		}
	}
	fail:
	deinit_functionfs();
	end:st.ctrl_tid=0;
	st.ctl_stopped=true;
	return NULL;
}

int fastboot_init(const char*path){
	int fd,e=0;
	if(usb_fd_ctl>=0)return -1;
	if((fd=open(path,O_DIRECTORY|O_RDONLY))<0)
		return ERROR(-1,"open functionfs failed");
	st.run=true;
	if((usb_fd_ctl=openat(fd,"ep0",O_RDWR))<0)
		EGOTO(-1,"cannot open control endpoint");
	st.ctl_stopped=true;
	pthread_create(&st.ctrl_tid,NULL,fastboot_ctrl_thread,NULL);
	if(full_write(usb_fd_ctl,&desc,sizeof(desc))<0)
		EGOTO(-1,"write usb descriptors failed");
	if(full_write(usb_fd_ctl,&strings,sizeof(strings))<0)
		EGOTO(-1,"write usb strings failed");
	if((usb_fd_out=openat(fd,"ep1",O_RDWR))<0)
		EGOTO(-1,"cannot open bulk-out endpoint");
	if((usb_fd_in=openat(fd,"ep2",O_RDWR))<0)
		EGOTO(-1,"cannot open bulk-in endpoint");
	if(!fb_blkid_cache)blkid_get_cache(&fb_blkid_cache,NULL);
	start_io_thread();
	fastboot_init_variables();
	close(fd);
	DEBUG("fastboot initialized\n");
	return e;
	fail:
	close_functionfs();
	close(fd);
	st.run=false;
	return e;
}

int fastboot_stop(){
	if(!st.run)ERET(ESRCH);
	close_functionfs();
	while(!st.io_stopped&&!st.ctl_stopped);
	return 0;
}
