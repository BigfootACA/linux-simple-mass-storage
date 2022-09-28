#include"config.h"
#include"init.h"
#include<linux/fb.h>
#include<pthread.h>

#if DISPLAY_SVG == 1
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include"nanosvg.h"
#include"nanosvgrast.h"
#endif

#if DISPLAY_STB == 1
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include"stb_image.h"
#include"stb_image_resize.h"
#endif

#if DISPLAY_INIT == 1
static int fbdev_fd=-1;
static void*fbdev_mem=NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
#endif
#if DISPLAY_IMAGE == 1 && DISPLAY_JUMP == 1
static int img_w=0,img_h=0;
static void*img=NULL;
static pthread_mutex_t img_lock;
static pthread_t jump_thread_tid=0;
struct jump_pos{int dir,last,step;};
#endif

#if DISPLAY_IMAGE == 1
static void draw_img(
	bool swap,
	void*src,int src_s,int src_w,int src_h,
	void*dst,int dst_s,int dst_x,int dst_y,int dst_w,int dst_h
){
	uint8_t*mem=src,*dest=dst+(dst_y*dst_w*dst_s);
	for(int y=0;y<MIN(src_h,dst_h-dst_y);y++){
		dest+=dst_x*dst_s;
		int xw=MIN(src_w,dst_w-dst_x);
		for(int x=0;x<xw;x++){
			dest[0]=mem?mem[swap?2:0]:0;
			dest[1]=mem?mem[1]:0;
			dest[2]=mem?mem[swap?0:2]:0;
			dest+=dst_s;
			if(mem)mem+=src_s;
		}
		dest+=(dst_w-dst_x-xw)*dst_s;
	}
}

#if DISPLAY_JUMP == 1
static void calc_pos(bool init,struct jump_pos*pos,int max){
	if(init){
		pos->last=rnd(0,max);
		pos->dir=rnd(0,1);
		pos->step=rnd(5,max/10);
		return;
	}
	if(pos->dir){
		pos->last-=pos->step;
		if(pos->last<=0){
			pos->last=0,pos->dir=0;
			pos->step=rnd(5,max/10);
		}
	}else{
		pos->last+=pos->step;
		if(pos->last>=max){
			pos->last=max,pos->dir=1;
			pos->step=rnd(5,max/10);
		}
	}
}

void*jump_thread(void*d __attribute__((unused))){
	struct jump_pos px={0,-1,0},py={0,-1,0};
	if(!fbdev_mem)return NULL;
	while(true){
		pthread_mutex_lock(&img_lock);
		if(
			img&&
			img_w>0&&img_w<(int)vinfo.xres&&
			img_h>0&&img_h<(int)vinfo.yres
		){
			int dw=vinfo.xres-img_w,dh=vinfo.yres-img_h;
			if(px.last<0||px.last<0){
				memset(fbdev_mem,0,finfo.smem_len);
				calc_pos(true,&px,dw);
				calc_pos(true,&py,dh);
			}
			#if DISPLAY_FLUSH == 1
			else draw_img(
				false,NULL,4,img_w,img_h,
				fbdev_mem,vinfo.bits_per_pixel/8,
				px.last,py.last,vinfo.xres,vinfo.yres
			);
			#endif
			calc_pos(false,&px,dw);
			calc_pos(false,&py,dh);
			draw_img(
				true,img,4,img_w,img_h,
				fbdev_mem,vinfo.bits_per_pixel/8,
				px.last,py.last,vinfo.xres,vinfo.yres
			);
			ioctl(fbdev_fd,FBIOPAN_DISPLAY,&vinfo);
		}else px.last=-1,px.last=-1;
		pthread_mutex_unlock(&img_lock);
		usleep(50000);
	}
}
#endif
#endif

#if DISPLAY_INIT == 1
int init_fbdev(void){
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
	#if DISPLAY_IMAGE == 1 && DISPLAY_JUMP == 1
	if(!jump_thread_tid){
		pthread_mutex_init(&img_lock,NULL);
		pthread_create(&jump_thread_tid,NULL,jump_thread,NULL);
	}
	#endif
	return 0;
	fail:
	if(fbdev_fd>0)close(fbdev_fd);
	if(fbdev_mem)munmap(fbdev_mem,finfo.smem_len);
	fbdev_fd=-1,fbdev_mem=NULL;
	return e;
}
#endif

#if DISPLAY_IMAGE == 1
int draw_svg_splash(const char*path){
	int e=0,pw=0,ph=0;
	char*ext=NULL;
	uint8_t*mem=NULL,*x=NULL;
	#if DISPLAY_STB == 1
	void*buf=NULL;
	#endif
	#if DISPLAY_SVG == 1
	NSVGimage*m=NULL;
	NSVGrasterizer*rast=NULL;
	#endif
	if(!fbdev_mem)return -1;
	memset(fbdev_mem,0,finfo.smem_len);
	if(!path||!(ext=strrchr(path,'.')))return -1;
	#if DISPLAY_SVG == 1
	else if(strcasecmp(ext+1,"svg")==0){
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
		#if DISPLAY_JUMP == 1
		ps/=5;
		#endif
		pw=m->width*ps,ph=m->height*ps;
		DEBUG("svg scale: %0.2f (%dx%d)\n",ps,pw,ph);
		if(!(x=mem=malloc(pw*ph*4)))EGOTO(-1,"alloc for svg failed");
		memset(mem,0,pw*ph*4);
		nsvgRasterize(rast,m,0,0,ps,mem,pw,ph,pw*4);
		if(rast)nsvgDeleteRasterizer(rast);
		if(m)nsvgDelete(m);
		rast=NULL,m=NULL;
	}
	#endif
	#if DISPLAY_STB == 1
	else if(ext){
		int sw=0,sh=0;
		DEBUG("load image %s\n",path);
		if(!(buf=stbi_load(path,&sw,&sh,NULL,4)))
			EGOTO(-1,"read image failed");
		DEBUG("image size: %dx%d\n",sw,sh);
		if(sw<=0||sh<=0)EGOTO(-1,"read image invalid");
		float ps=MIN(
			vinfo.xres*DISPLAY_SCALE/sw,
			vinfo.yres*DISPLAY_SCALE/sh
		);
		#if DISPLAY_JUMP == 1
		ps/=5;
		#endif
		pw=sw*ps,ph=sh*ps;
		DEBUG("image scale: %0.2f (%dx%d)\n",ps,pw,ph);
		if(!(x=mem=malloc(pw*ph*4)))EGOTO(-1,"alloc for image failed");
		memset(mem,0,pw*ph*4);
		if(!stbir_resize_uint8(buf,sw,sh,0,mem,pw,ph,0,4))
			EGOTO(-1,"resize image failed");
		if(buf)free(buf);
		buf=NULL;
	}
	#endif
	else EGOTO(-1,"unsupported file %s",path);
	#if DISPLAY_JUMP == 1
	pthread_mutex_lock(&img_lock);
	if(img)free(img);
	img=mem,img_w=pw,img_h=ph;
	pthread_mutex_unlock(&img_lock);
	#else
	int px=(vinfo.xres-pw)/2,py=(vinfo.yres-ph)/2;
	DEBUG("svg draw pos: %dx%d\n",px,py);
	draw_img(
		true,mem,4,pw,ph,fbdev_mem,
		vinfo.bits_per_pixel/8,px,py,
		vinfo.xres,vinfo.yres
	);
	ioctl(fbdev_fd,FBIOPAN_DISPLAY,&vinfo);
	if(x)free(x);
	#endif
	return 0;
	fail:
	memset(fbdev_mem,0,finfo.smem_len);
	ioctl(fbdev_fd,FBIOPAN_DISPLAY,&vinfo);
	#if DISPLAY_SVG == 1
	if(rast)nsvgDeleteRasterizer(rast);
	if(m)nsvgDelete(m);
	#endif
	#if DISPLAY_STB == 1
	if(buf)free(buf);
	#endif
	if(x)free(x);
	return e;
}
#endif
