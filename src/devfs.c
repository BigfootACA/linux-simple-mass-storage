#include"config.h"
#include"init.h"

#if USE_DEVTMPFS == 0
static int mkdir_res(char*path){
	if(!path)return -1;
	if(strcmp(path,"/")==0)return 0;
	bool slash=false;
	for(size_t x=0;x<=strlen(path);x++)
		if(path[x]=='/')slash=true;
	char s[PATH_MAX]={0},*x;
	strcpy(s,path);
	if(!(x=dirname(s)))return -1;
	mkdir(x,0755);
	if(access(x,F_OK)==0)slash=false;
	return slash?mkdir_res(x):0;
}

static void do_mknod(int dir,int type){
	int major=-1,minor=-1,uevent;
	char*name=NULL,kb[PATH_MAX]={0},vb[PATH_MAX]={0},buff[PATH_MAX]={0};
	size_t idx,xi=0;
	ssize_t r;
	bool expr=false;
	if((uevent=openat(dir,"uevent",O_RDONLY))<0)return;
	while((r=read(uevent,&buff,PATH_MAX))>0){
		for(idx=0;idx<=(size_t)r;idx++){
			char c=buff[idx];
			if(c=='=')kb[xi]=0,expr=true,xi=0;
			else if(c=='\n'||c=='\r'){
				vb[xi]=0;
				if(xi>0&&expr){
					if(strcmp(kb,"DEVNAME")==0&&!name)name=strdup(vb);
					if(strcmp(kb,"MAJOR")==0&&major<=0)major=atoi(vb);
					if(strcmp(kb,"MINOR")==0&&minor<=0)minor=atoi(vb);
				}
				xi=0,kb[xi]=0,vb[xi]=0,expr=false;
			}else if(expr)vb[xi++]=c;
			else kb[xi++]=c;
		}
		memset(&buff,0,PATH_MAX);
	}
	close(uevent);
	if(name&&major>=0&&minor>=0){
		char dev[PATH_MAX]={0};
		snprintf(dev,PATH_MAX-1,PATH_DEV"/%s",name);
		mkdir_res(dev);
		mknod(dev,type|0600,makedev(major,minor));
	}
	if(name)free(name);
}

static void scan_devices(int dir,int type){
	int f=-1;
	DIR*d=fdopendir(dir);
	if(!d){
		close(dir);
		return;
	}
	struct dirent*r=NULL;
	while((r=readdir(d))){
		if(r->d_name[0]=='.'||r->d_type!=DT_LNK)continue;
		if((f=openat(dir,r->d_name,O_DIR))<0)continue;
		do_mknod(f,type);
		close(f);
	}
	closedir(d);
}

void init_devfs(void){
	int fd=open(PATH_SYS_DEV,O_DIR);
	if(fd<0)FAILURE(-1,"open "PATH_SYS_DEV" failed");
	scan_devices(openat(fd,"char",O_DIR),S_IFCHR);
	scan_devices(openat(fd,"block",O_DIR),S_IFBLK);
	close(fd);
}
#endif
