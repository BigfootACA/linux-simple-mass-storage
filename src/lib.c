#include"config.h"
#include"init.h"
#include<ctype.h>
#include<sys/time.h>
#include<sys/mount.h>

const char*size_units[]={"B","KiB","MiB","GiB","TiB","PiB","EiB","ZiB","YiB",NULL};

ssize_t write_file(int fd,const char*content,const char*file,...){
	va_list va;
	char path[PATH_MAX];
	if(!content||!file)return -1;
	memset(path,0,sizeof(path));
	va_start(va,file);
	vsnprintf(path,sizeof(path)-1,file,va);
	va_end(va);
	int f=openat(fd,path,O_WRONLY);
	if(f<0)return -1;
	ssize_t r=write(f,content,strlen(content));
	close(f);
	return r;
}

ssize_t writef_file(int fd,const char*file,const char*content,...){
	int f;
	va_list va;
	ssize_t r=-1;
	char*cont=NULL;
	if(!content||!file)return -1;
	va_start(va,content);
	vasprintf(&cont,content,va);
	va_end(va);
	if(!cont)return -1;
	if((f=openat(fd,file,O_WRONLY))>=0){
		r=write(f,cont,strlen(cont));
		close(f);
	}
	free(cont);
	return r;
}

ssize_t read_file(int fd,char*buff,size_t len,const char*file,...){
	va_list va;
	char path[PATH_MAX];
	if(!buff||!file||len<=0)return -1;
	memset(path,0,sizeof(path));
	va_start(va,file);
	vsnprintf(path,sizeof(path)-1,file,va);
	va_end(va);
	int f=openat(fd,path,O_RDONLY);
	if(f<0)return -1;
	memset(buff,0,len);
	ssize_t s=read(f,buff,len-1);
	if(s>0){
		if(buff[s-1]=='\n')buff[--s]=0;
		if(buff[s-1]=='\r')buff[--s]=0;
	}
	close(f);
	return s;
}

void xmount(const char*src,const char*dir,const char*type,unsigned long flag,const char*data){
	if(!src||!dir||!type)return;
	errno=0;
	int r=mount(src,dir,type,flag,data);
	if(r==0)DEBUG("mount %s(%s) to %s success\n",src,type,dir);
	else FAILURE(-1,"mount %s(%s) to %s failed",src,type,dir);
}

ssize_t full_write(int fd,const void*data,size_t len){
	ssize_t sent;
	size_t rxl=len;
	void*rxd=(void*)data;
	do{
		errno=0;
		sent=write(fd,rxd,rxl);
		if(sent<0&&errno!=EAGAIN&&errno!=EINTR)return sent;
		rxl-=sent,rxd+=sent;
	}while(rxl>0);
	return (ssize_t)len;
}

ssize_t full_read(int fd,void*data,size_t len){
	ssize_t sent;
	size_t rxl=len;
	void*rxd=data;
	do{
		errno=0;
		sent=read(fd,rxd,rxl);
		if(sent<0&&errno!=EAGAIN&&errno!=EINTR)return sent;
		rxl-=sent,rxd+=sent;
	}while(rxl>0);
	return (ssize_t)len;
}

uint64_t get_now_ms(){
	struct timeval tv_start;
	gettimeofday(&tv_start,NULL);
	return (tv_start.tv_sec*1000000+tv_start.tv_usec)/1000;
}

size_t s_strlcpy(char *buf,const char*src,size_t len){
	char*d0=buf;
	if(!len--)goto finish;
	for(;len&&(*buf=*src);len--,src++,buf++);
	*buf=0;
	finish:
	return buf-d0+strlen(src);
}

size_t s_strlcat(char*buf,const char*src,size_t len){
	size_t slen=strnlen(buf,len);
	return slen==len?
		(slen+strlen(src)):
		(slen+s_strlcpy(buf+slen,src,len-slen));
}

void*s_memdup(void*mem,size_t len){
	void*dup=malloc(len);
	if(!dup)EPRET(ENOMEM);
	memcpy(dup,mem,len);
	return dup;
}

const char*format_size(
	char*buf,size_t len,
	size_t val,
	size_t block,
	size_t display
){
	int unit=0;
	memset(buf,0,len);
	if(val==0)return strncpy(buf,"0",len-1);
	if(block>1)val*=block;
	if(display)val+=display/2,val/=display;
	else while((val>=1024))val/=1024,unit++;
	snprintf(buf,len-1,"%zu %s",val,size_units[unit]);
	return buf;
}


bool is_virt_dir(const struct dirent*d){
	return
		strcmp(d->d_name,".")==0||
		strcmp(d->d_name,"..")==0;
}

int remove_folders(int dfd,int flags){
	struct dirent*ent;
	DIR*dir;
	if(!(dir=fdopendir(dfd)))return -1;
	while((ent=readdir(dir)))
		if(!is_virt_dir(ent)&&ent->d_type==DT_DIR)
			unlinkat(dfd,ent->d_name,flags);
	free(dir);
	return 0;
}

static struct mount_item**_array_add_entry(struct mount_item**array,struct mount_item*entry){
	size_t idx=0,s;
	if(array)while(array[idx++]);
	else idx++;
	s=sizeof(struct mount_item*)*(idx+1);
	if(!(array=array?realloc(array,s):malloc(s)))return NULL;
	array[idx-1]=entry;
	array[idx]=NULL;
	return array;
}

long parse_long(char*str,long def){
	if(!str)return def;
	errno=0;
	char*end;
	long val=strtol(str,&end,0);
	return errno!=0||end==str?def:val;
}

int parse_int(char*str,int def){
	return (int)parse_long(str,(int)def);
}

char**args2array(char*source,char del){
	if(!source)return NULL;
	int item=0;
	size_t sz=sizeof(char)*(strlen(source)+1);
	char**array=NULL,*buff=NULL;
	if(!(buff=malloc(sz)))goto fail;
	if(!(array=malloc((sizeof(char*)*(item+2)))))goto fail;
	char*scur=source,*dcur=buff,**arr=NULL,b;
	memset(buff,0,sz);
	array[item]=dcur;
	while(*scur>0)if(del==0?isspace(*scur):del==*scur){
		scur++;
		if(*array[item]){
			dcur++,item++;
			if(!(arr=realloc(array,(sizeof(char*)*(item+2)))))goto fail;
			array=arr;
			array[item]=dcur;
		}
	}else if(*scur=='"'||*scur=='\''){
		b=*scur;
		while(*scur++&&*scur!=b){
			if(*scur=='\\')scur++;
			*dcur++=*scur;
		}
		dcur++,scur++;
	}else *dcur++=*scur++;
	if(!*array[item]){
		array[item]=NULL;
		if(item==0){
			free(buff);
			buff=NULL;
		}
	}
	array[++item]=NULL;
	if(!(arr=malloc((sizeof(char*)*(item+2)))))goto fail;
	memset(arr,0,(sizeof(char*)*(item+2)));
	for(int t=0;array[t];t++)arr[t]=array[t];
	free(array);
	return arr;
	fail:
	if(buff)free(buff);
	if(array)free(array);
	buff=NULL,array=NULL;
	return NULL;
}

int char_array_len(char**arr){
	int i=-1;
	if(arr)while(arr[++i]);
	return i;
}

struct mount_item**read_proc_mounts(){
	FILE*f=fopen(PATH_PROC"/self/mounts","r");
	if(!f)return NULL;
	size_t bs=sizeof(struct mount_item);
	struct mount_item**array=NULL,**arr,*buff;
	char line[BUFSIZ];
	while(!feof(f)){
		memset(line,0,BUFSIZ);
		fgets(line,BUFSIZ,f);
		if(!line[0])continue;
		char**c=args2array(line,0);
		if(!c||char_array_len(c)!=6)continue;
		if(!(buff=malloc(bs))){
			if(array)free_mounts(array);
			return NULL;
		}
		memset(buff,0,bs);
		buff->source=c[0];
		buff->target=c[1];
		buff->type=c[2];
		buff->options=args2array(c[3],',');
		buff->freq=parse_int(c[4],0);
		buff->passno=parse_int(c[5],0);
		free(c);
		if(!(arr=_array_add_entry(array,buff))){
			free_mounts(array);
			array=NULL;
			free(buff);
			return NULL;
		}else array=arr;
	}
	return array;
}

void free_mount_item(struct mount_item*m){
	if(!m)return;
	if(m->source)free(m->source);
	if(m->options){
		if(m->options[0])free(m->options[0]);
		free(m->options);
	}
	memset(m,0,sizeof(struct mount_item));
	free(m);
}

void free_mounts(struct mount_item**c){
	if(!c)return;
	struct mount_item*s;
	size_t l=0;
	while((s=c[l++]))free_mount_item(s);
	free(c);
}
