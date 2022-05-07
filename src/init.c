#include"config.h"
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>
#include<stdint.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<unistd.h>
#include<limits.h>
#include<libgen.h>
#include<stdbool.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<sys/mount.h>
#include<sys/sysmacros.h>
#include<linux/fb.h>
#define O_DIR O_RDONLY|O_DIRECTORY
#ifndef MIN
#define MIN(a,b)((b)>(a)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)((b)<(a)?(a):(b))
#endif

#if DISPLAY_SVG == 1
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include"nanosvg.h"
#include"nanosvgrast.h"
#endif

#define EGOTO(err,str...) {\
	e=ERROR(err,str);\
	goto fail;\
}

#if USE_TCM == 0 && USE_MASS_STORAGE == 0
#error no backend method
#endif
#if USE_TCM == 1 && USE_MASS_STORAGE == 1
#error conflict backend method
#endif

static int fbdev_fd=-1;
static int out_fd=STDERR_FILENO;
static void*fbdev_mem=NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;

#if USE_DEBUG == 1
#define DEBUG(str...) debug(__func__,str)
static int debug(const char*tag,const char*str,...){
	va_list va;
	char buf[BUFSIZ]={0};
	snprintf(buf,sizeof(buf)-1,"<6>%s: ",tag);
	size_t tl=strlen(buf);
	va_start(va,str);
	vsnprintf(buf+tl,sizeof(buf)-tl-1,str,va);
	va_end(va);
	write(out_fd,buf,strlen(buf));
	return 0;
}
#else
#define DEBUG(str...)
#endif

#if USE_ERROR == 1
static int _error(int r,int l,const char*tag,const char*str,va_list va){
	int err=errno;
	ssize_t tl;
	ssize_t bs;
	char buf[BUFSIZ]={0};
	snprintf(buf,sizeof(buf)-4,"<%d>%s: ",l,tag);
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

static inline int error(int r,const char*tag,const char*str,...){
	va_list va;
	va_start(va,str);
	int e=_error(r,3,tag,str,va);
	va_end(va);
	return e;
}

static inline int failure(int r,const char*tag,const char*str,...){
	va_list va;
	va_start(va,str);
	int e=_error(r,0,tag,str,va);
	va_end(va);
	_exit(e);
	return e;
}
#define ERROR(err,str...) error(err,__func__,str)
#define FAILURE(err,str...) failure(err,__func__,str)
#else
#define ERROR(err,str...) err
#define FAILURE(err,str...) exit(err);
#endif

static void xmount(const char*src,const char*dir,const char*type,unsigned long flag,const char*data){
	if(!src||!dir||!type)return;
	errno=0;
	int r=mount(src,dir,type,flag,data);
	if(r==0)DEBUG("mount %s(%s) to %s success\n",src,type,dir);
	else FAILURE(-1,"mount %s(%s) to %s failed",src,type,dir);
}

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

static void init_devfs(void){
	int fd=open(PATH_SYS_DEV,O_DIR);
	if(fd<0)FAILURE(-1,"open "PATH_SYS_DEV" failed");
	scan_devices(openat(fd,"char",O_DIR),S_IFCHR);
	scan_devices(openat(fd,"block",O_DIR),S_IFBLK);
	close(fd);
}
#endif

static void init_console(void){
	for(int i=0;i<65535;i++)close(i);
	int fd=open(PATH_CONSOLE,O_RDWR);
	if(fd<0)fd=open(PATH_NULL,O_RDWR);
	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);
	if((out_fd=open(PATH_KMSG,O_RDWR))<0)out_fd=fd;
	DEBUG("console opened\n");
}

static void init_fs(void){
	chdir("/");
	mkdir(PATH_DEV,0755);
	mkdir(PATH_SYS,0555);
	#if USE_DEVTMPFS == 0
	xmount("devfs",PATH_DEV,"tmpfs",MS_NOSUID,"mode=755");
	#else
	xmount("devfs",PATH_DEV,"devtmpfs",MS_NOSUID,"mode=755");
	#endif
	xmount("sysfs",PATH_SYS,"sysfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
	xmount("configfs",PATH_CONFIGFS,"configfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
	#if USE_DEVTMPFS == 0
	init_devfs();
	#endif
	init_console();
}

static int init_fbdev(void){
	int e=0;
	if((fbdev_fd=open(PATH_FBDEV,O_RDWR))<0)
		EGOTO(-1,"open fbdev failed");
	if(ioctl(fbdev_fd,FBIOGET_FSCREENINFO,&finfo)==-1)
		EGOTO(-1,"ioctl FBIOGET_FSCREENINFO failed");
	if(ioctl(fbdev_fd,FBIOGET_VSCREENINFO,&vinfo)==-1)
		EGOTO(-1,"ioctl FBIOGET_VSCREENINFO failed");
	if(finfo.smem_len<=0)EGOTO(-1,"invalid fbdev memory size");
	if(vinfo.xres<=0)EGOTO(-1,"invalid width size");
	if(vinfo.yres<=0)EGOTO(-1,"invalid height size");
	if(vinfo.bits_per_pixel!=32&&vinfo.bits_per_pixel!=24)
		EGOTO(-1,"unsupported bpp %d",vinfo.bits_per_pixel);
	fbdev_mem=(char*)mmap(0,finfo.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fbdev_fd,0);
	if((intptr_t)fbdev_mem==-1||!fbdev_mem)EGOTO(-1,"mmap failed");
	memset(fbdev_mem,0,finfo.smem_len);
	ioctl(fbdev_fd,FBIOPAN_DISPLAY,&vinfo);
	ioctl(fbdev_fd,FBIOBLANK,0);
	DEBUG("fbdev screen size: %dx%d\n",vinfo.xres,vinfo.yres);
	return 0;
	fail:
	if(fbdev_fd>0)close(fbdev_fd);
	if(fbdev_mem)munmap(fbdev_mem,finfo.smem_len);
	fbdev_fd=-1,fbdev_mem=NULL;
	return e;
}

#if DISPLAY_SVG == 1
static int draw_svg_splash(const char*path){
	int e=0;
	NSVGimage*m=NULL;
	NSVGrasterizer*rast=NULL;
	uint8_t*mem=NULL,*x=NULL;
	if(!fbdev_mem)return -1;
	memset(fbdev_mem,0,finfo.smem_len);
	DEBUG("load svg %s\n",path);
	if(!(m=nsvgParseFromFile(path,"px",SVG_DPI)))
		EGOTO(-1,"read svg failed");
	DEBUG("svg size: %0.0fx%0.0f\n",m->width,m->height);
	if(!(rast=nsvgCreateRasterizer()))
		EGOTO(-1,"create rasterizer failed");
	float ps=MIN(
		vinfo.xres*DISPLAY_SCALE/m->width,
		vinfo.yres*DISPLAY_SCALE/m->height
	);
	int pw=m->width*ps,ph=m->height*ps;
	DEBUG("svg scale: %0.2f (%dx%d)\n",ps,pw,ph);
	if(!(x=mem=malloc(pw*ph*4)))EGOTO(-1,"alloc for svg failed");
	memset(mem,0,pw*ph*4);
	nsvgRasterize(rast,m,0,0,ps,mem,pw,ph,pw*4);
	int px=(vinfo.xres-pw)/2,py=(vinfo.yres-ph)/2;
	int bs=vinfo.bits_per_pixel/8;
	DEBUG("svg draw pos: %dx%d\n",px,py);
	uint8_t*dst=fbdev_mem+(py*vinfo.xres*bs);
	for(int y=0;y<ph;y++){
		dst+=px*bs;
		for(int x=0;x<pw;x++){
			dst[0]=mem[2];
			dst[1]=mem[1];
			dst[2]=mem[0];
			dst+=bs,mem+=4;
		}
		dst+=(vinfo.xres-px-pw)*bs;
	}
	ioctl(fbdev_fd,FBIOPAN_DISPLAY,&vinfo);
	if(rast)nsvgDeleteRasterizer(rast);
	if(m)nsvgDelete(m);
	if(x)free(x);
	return 0;
	fail:
	memset(fbdev_mem,0,finfo.smem_len);
	ioctl(fbdev_fd,FBIOPAN_DISPLAY,&vinfo);
	if(rast)nsvgDeleteRasterizer(rast);
	if(m)nsvgDelete(m);
	if(x)free(x);
	return e;
}
#endif

static ssize_t write_file(int fd,const char*content,const char*file,...){
	va_list va;
	char path[PATH_MAX]={0};
	if(!content||!file)return -1;
	va_start(va,file);
	vsnprintf(path,sizeof(path)-1,file,va);
	va_end(va);
	int f=openat(fd,path,O_WRONLY);
	if(f<0)return -1;
	ssize_t r=write(f,content,strlen(content));
	close(f);
	return r;
}

static ssize_t read_file(int fd,char*buff,size_t len,const char*file,...){
	va_list va;
	char path[PATH_MAX]={0};
	if(!buff||!file||len<=0)return -1;
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

static void check_env(void){
	if(getpid()!=1)FAILURE(1,"Not PID 1");
	if(getuid()!=0)FAILURE(1,"Not UID 0");
	if(getgid()!=0)FAILURE(1,"Not GID 0");
}

static int start_gadget(int g){
	int e=0;
	#ifndef DEFAULT_UDC
	DIR*bs=NULL;
	struct dirent*d;
	char udc[PATH_MAX];
	bool done=false,notice=false;
	errno=0;
	if(!(bs=opendir(PATH_UDC)))
		EGOTO(-1,"open "PATH_UDC" failed");
	while(1){
		while((d=readdir(bs))){
			if(d->d_name[0]=='.'||d->d_type!=DT_LNK)continue;
			memset(udc,0,sizeof(udc));
			strcpy(udc,d->d_name);
			DEBUG("found udc %s\n",udc);
			strcat(udc,"\n");
			if(write_file(g,udc,"UDC")<=0)
				EGOTO(-1,"write UDC failed");
			done=true;
			break;
		}
		if(done)break;
		else{
			if(!notice)DEBUG("no available udc found, wait\n");
			notice=true;
			seekdir(bs,0);
			sleep(5);
		}
	}
	#else
	if(write_file(g,DEFAULT_UDC"\n","UDC")<=0)
		EGOTO(-1,"write UDC failed");
	#endif
	DEBUG("gadget started\n");
	fail:
	if(bs)closedir(bs);
	return e;
}

#if USE_MASS_STORAGE == 1
static void gadget_add_mass_storage(int g,int b,long size,char*name,char*path){
	char func[PATH_MAX]="functions/mass_storage.";
	strcat(func,name);
	size_t l=strlen(func);
	func[l]=0;
	mkdirat(g,func,0755);
	strcat(func,"/lun.0/file");
	func[l+11]=0;
	write_file(g,path,func);
	func[l]=0;
	symlinkat(func,b,name);
}
#endif

#if USE_TCM == 1
static void gadget_add_target(char*name,char*path){
	static int lun=0;
	char control[PATH_MAX*2]={0};
	char lun_path[PATH_MAX]={0};
	char blk_path[PATH_MAX]={0};
	snprintf(control,sizeof(control)-1,"udev_path=%s\n",path);
	snprintf(blk_path,sizeof(blk_path)-1,PATH_TARGET_CORE"/%s",name);
	snprintf(lun_path,sizeof(lun_path)-1,PATH_TPGT"/lun/lun_%d",lun);
	mkdir(blk_path,0755);
	write_file(AT_FDCWD,control,"%s/control",blk_path);
	write_file(AT_FDCWD,"1\n","%s/enable",blk_path);
	write_file(AT_FDCWD,name,"%s/wwn/product_id",blk_path);
	write_file(AT_FDCWD,"Qualcomm","%s/wwn/vendor_id",blk_path);
	mkdir(lun_path,0755);
	strcat(lun_path,"/virtual_scsi_port");
	symlink(blk_path,lun_path);
	lun++;
}
#endif

static int gadget_add_blocks(int g,int b){
	DIR*bs=NULL;
	struct stat st;
	struct dirent*d;
	int blk=0,e=0,s=-1;
	char ss[64],path[PATH_MAX];
	errno=0;
	#if USE_TCM == 1
	mkdirat(g,"functions/tcm.target",0755);
	mkdir(PATH_TARGET_CORE,0755);
	mkdir(PATH_FABRIC,0755);
	mkdir(PATH_NAA,0755);
	mkdir(PATH_TPGT,0755);
	#endif
	if(!(s=open(PATH_BLOCK,O_DIR))||!(bs=fdopendir(s)))
		EGOTO(-1,"open "PATH_BLOCK" failed");
	while((d=readdir(bs))){
		if(d->d_name[0]=='.'||d->d_type!=DT_LNK)continue;
		if(read_file(s,ss,sizeof(ss),"%s/size",d->d_name)<=0)continue;
		if(atol(ss)<=0)continue;
		memset(path,0,sizeof(path));
		snprintf(path,sizeof(path)-1,PATH_DEV"/%s",d->d_name);
		if(stat(path,&st)!=0)continue;
		if(!S_ISBLK(st.st_mode))continue;
		DEBUG("found block %s\n",d->d_name);
		#if USE_MASS_STORAGE == 1
		gadget_add_mass_storage(b,d->d_name,path);
		#endif
		#if USE_TCM == 1
		gadget_add_target(d->d_name,path);
		#endif
		blk++;
	}
	if(blk<=0)EGOTO(-1,"no any block devices found");
	DEBUG("added %d blocks\n",blk);
	write_file(AT_FDCWD,TARGET_NAA"\n",PATH_TPGT"/nexus");
	write_file(AT_FDCWD,"1\n",PATH_TPGT"/enable");
	symlinkat("functions/tcm.target",b,"target");
	fail:
	if(bs)closedir(bs);
	return e;
}

static int init_gadget(void){
	int c=-1,g=-1,e=0,b=-1;
	if((c=open(PATH_GADGET,O_DIR))<0)
		return ERROR(-1,"open gadget configfs failed");
	DEBUG("init usb gadget\n");
	mkdirat(c,"gadget",0755);
	if((g=openat(c,"gadget",O_DIR))<0)
		EGOTO(-1,"create gadget failed");
	fchdir(g);
	write_file(g,USB_ID_VENDOR"\n","idVendor");
	write_file(g,USB_ID_PRODUCT"\n","idProduct");
	write_file(g,"0x0320\n","bcdUSB");
	mkdirat(g,"strings/0x409",0755);
	write_file(g,USB_MANUFACTURER"\n","strings/0x409/manufacturer");
	write_file(g,USB_PRODUCT"\n","strings/0x409/product");
	write_file(g,SERIAL_NUMBER"\n","strings/0x409/serialnumber");
	write_file(g,"1\n","os_desc/use");
	write_file(g,"0x1\n","os_desc/b_vendor_code");
	write_file(g,"MSFT100\n","os_desc/qw_sign");
	mkdirat(g,"configs/a.1",0755);
	if((b=openat(g,"configs/a.1",O_DIR))<0)
		EGOTO(-1,"open gadget config failed");
	mkdirat(b,"strings/0x409",0755);
	write_file(b,USB_CONFIG "\n","strings/0x409/configuration");
	if(gadget_add_blocks(g,b)!=0)EGOTO(-1,NULL);
	if(start_gadget(g)!=0)EGOTO(-1,NULL);
	chdir("/");
	DEBUG("gadget initialized\n");
	fail:
	if(b>=0)close(b);
	if(c>=0)close(c);
	if(g>=0)close(g);
	return e;
}

int main(void){
	(void)check_env();
	(void)init_fs();
	(void)init_fbdev();
	#if DISPLAY_SVG == 1
	(void)draw_svg_splash(SVG_PRE_PATH);
	#endif
	#if USE_BUSYBOX_ASH == 1
	if(fork()==0)_exit(execl("/busybox","ash",NULL));
	#endif
	(void)init_gadget();
	#if DISPLAY_SVG == 1
	(void)draw_svg_splash(SVG_POST_PATH);
	#endif
	while(1)sleep(3600);
}
