/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"config.h"
#include"init.h"

static bool kmsg_out=false;
static int out_fd=STDERR_FILENO;

#if DEBUG_TAG == 1
static char*append_tag(
	char*buff,size_t len,
	const char*file __attribute__((unused)),
	const char*func,
	int line __attribute__((unused))
){
	#if DEBUG_LOCATION == 1
	if(file){
		s_strlcat(buff,file,len-1);
		s_strlcat(buff,"@",len-1);
	}
	#endif
	s_strlcat(buff,func,len-1);
	#if DEBUG_LOCATION == 1
	if(line>0){
		size_t l=strlen(buff);
		snprintf(buff+l,len-l-1,":%d",line);
	}
	#endif
	return buff;
}
#endif

#if USE_DEBUG == 1
int debug(const char*file,const char*tag,int line,const char*str,...){
	va_list va;
	char buf[BUFSIZ];
	memset(buf,0,sizeof(buf));
	if(kmsg_out)s_strlcat(buf,"<6>",sizeof(buf)-1);
	#if DEBUG_TAG == 1
	append_tag(buf,sizeof(buf),file,tag,line);
	s_strlcat(buf,": ",sizeof(buf)-1);
	#endif
	size_t tl=strlen(buf);
	va_start(va,str);
	vsnprintf(buf+tl,sizeof(buf)-tl-1,str,va);
	va_end(va);
	write(out_fd,buf,strlen(buf));
	return 0;
}
#endif

#if USE_ERROR == 1
static int _error(
	int r,int l,
	const char*file,
	const char*tag,
	int line,
	const char*str,
	va_list va
){
	int err=errno;
	ssize_t tl,bs;
	char buf[BUFSIZ];
	memset(buf,0,sizeof(buf));
	if(kmsg_out)snprintf(buf,sizeof(buf)-1,"<%d>",l);
	#if DEBUG_TAG == 1
	append_tag(buf,sizeof(buf),file,tag,line);
	s_strlcat(buf,": ",sizeof(buf)-1);
	#endif
	tl=strlen(buf),bs=sizeof(buf)-tl-2;
	vsnprintf(buf+tl,bs,str,va);
	if(err!=0){
		tl=strlen(buf),bs=sizeof(buf)-tl-2;
		if(bs>0)snprintf(buf+tl,bs,": %s",strerror(err));
	}
	strcat(buf,"\n");
	write(out_fd,buf,strlen(buf));
	return r==0?err:r;
}

int error(
	int r,
	const char*file,
	const char*tag,
	int line,
	const char*str,...
){
	va_list va;
	va_start(va,str);
	int e=_error(r,3,file,tag,line,str,va);
	va_end(va);
	return e;
}

int failure(
	int r,
	const char*file,
	const char*tag,
	int line,
	const char*str,...
){
	va_list va;
	va_start(va,str);
	int e=_error(r,0,file,tag,line,str,va);
	va_end(va);
	_exit(e);
	return e;
}
#endif

void init_console(void){
	for(int i=0;i<65535;i++)close(i);
	int fd=open(PATH_CONSOLE,O_RDWR);
	if(fd<0)fd=open(PATH_NULL,O_RDWR);
	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);
	if((out_fd=open(PATH_KMSG,O_RDWR))<0)
		out_fd=fd,kmsg_out=true;
	DEBUG("console opened\n");
}
