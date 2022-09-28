#ifndef _INIT_H
#define _INIT_H
#include"config.h"
#include<time.h>
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
#include<sys/sysmacros.h>
#define O_DIR O_RDONLY|O_DIRECTORY
#ifndef MIN
#define MIN(a,b)((b)>(a)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)((b)<(a)?(a):(b))
#endif

#define ALIGN(val,alg) ((val)+(((alg)-(val))&((alg)-1)))
#define ARRLEN(x)(sizeof(x)/sizeof((x)[0]))
#define ERET(e){return -(errno=(e));}
#define EXRET(e){if(errno==0)errno=e;return -(errno);}
#define EPRET(e){errno=(e);return NULL;}
#define EGOTO(err,str...) {\
	e=ERROR(err,str);\
	goto fail;\
}
#define ERGOTO(err,str...){\
	ERROR(-1,str);\
	errno=err;\
	goto fail;\
}
#define EXGOTO(err,str...){\
	int _er=errno;\
	ERROR(-1,str);\
	errno=_er?:err;\
	goto fail;\
}
#define ENGOTO(err) {\
	errno=err;\
	goto fail;\
}

#define memdup s_memdup
extern void*s_memdup(void*mem,size_t len);
extern size_t s_strlcpy(char *buf,const char*src,size_t len);
extern size_t s_strlcat(char*buf,const char*src,size_t len);
#include"list.h"

struct gadget_config{
	uint16_t bcd_usb;
	uint16_t vendor_id;
	uint16_t product_id;
	char serialnumber[64];
	char manufacturer[64];
	char product[64];
};
struct mount_item{
	char*source,*target,*type,**options;
	int freq,passno;
};
enum mass_mode{
	MODE_NONE=0,
	MODE_AUTO,
	MODE_MASS_STORAGE,
	MODE_TARGET_CORE,
};
struct use_block{
	char path[PATH_MAX];
	char name[256];
	dev_t device;
	size_t size;
	size_t sector;
	int lun;
};
extern list*gadget_blocks;
extern enum mass_mode gadget_mode;
extern struct gadget_config gadget_config;
extern void init_console(void);
extern void init_devfs(void);
extern int init_fbdev(void);
extern int fastboot_init(const char*path);
extern int fastboot_stop(void);
extern int gadget_init(void);
extern int gadget_init(void);
extern int gadget_stop(void);
extern int gadget_start(void);
extern int gadget_cleanup(const char*name);
extern int gadget_scan_blocks(void);
extern void gadget_add_target_name(char*name);
extern int gadget_block_add_name(const char*name);
extern int gadget_block_add_path(const char*path);
extern int gadget_block_delete_name(const char*name);
extern int gadget_block_delete_path(const char*path);
extern struct use_block*gadget_lookup_block_by_name(const char*name);
extern struct use_block*gadget_lookup_block_by_path(const char*path);
extern ssize_t full_read(int fd,void*data,size_t len);
extern ssize_t full_write(int fd,const void*data,size_t len);
extern ssize_t read_file(int fd,char*buff,size_t len,const char*file,...)__attribute__((format(printf,4,5)));
extern ssize_t write_file(int fd,const char*content,const char*file,...)__attribute__((format(printf,3,4)));
extern ssize_t writef_file(int fd,const char*file,const char*content,...)__attribute__((format(printf,3,4)));
extern void xmount(const char*src,const char*dir,const char*type,unsigned long flag,const char*data);
extern int draw_svg_splash(const char*path);
extern int debug(const char*file,const char*tag,int line,const char*str,...)__attribute__((format(printf,4,5)));
extern int error(int r,const char*file,const char*tag,int line,const char*str,...)__attribute__((format(printf,5,6)));
extern int failure(int r,const char*file,const char*tag,int line,const char*str,...)__attribute__((format(printf,5,6)));
extern uint64_t get_now_ms(void);
extern struct mount_item**read_proc_mounts(void);
extern void free_mount_item(struct mount_item*m);
extern void free_mounts(struct mount_item**c);
extern int remove_folders(int dfd,int flags);
extern bool is_virt_dir(const struct dirent*d);
extern const char*format_size(char*buf,size_t len,size_t val,size_t block,size_t display);
static inline __attribute__((used)) int rnd(int min,int max){return rand()%(max-min+1)+min;}
static inline __attribute__((used)) int print(int fd,const char*cnt){return write(fd,cnt,strlen(cnt));}
#if USE_DEBUG == 1
#define DEBUG(str...) debug(__FILE__,__func__,__LINE__,str)
#else
#define DEBUG(str...)
#endif
#if USE_ERROR == 1
#define ERROR(err,str...) error(err,__FILE__,__func__,__LINE__,str)
#define FAILURE(err,str...) failure(err,__FILE__,__func__,__LINE__,str)
#else
#define ERROR(err,str...) err
#define FAILURE(err,str...) exit(err);
#endif
#endif
