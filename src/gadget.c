#include"config.h"
#include"init.h"
#include"list.h"
#include<linux/fs.h>
#include<sys/mount.h>

list*gadget_blocks=NULL;
enum mass_mode gadget_mode=MODE_AUTO;
static int gadget_fd=-1;
struct gadget_config gadget_config={
	.bcd_usb      = 0x0320,
	.vendor_id    = USB_ID_VENDOR,
	.product_id   = USB_ID_PRODUCT,
	.serialnumber = SERIAL_NUMBER,
	.manufacturer = USB_MANUFACTURER,
	.product      = USB_PRODUCT,
};

static bool block_cmp(list*l,void*d){
	struct use_block*b2=d;
	LIST_DATA_DECLARE(b1,l,struct use_block*);
	if(!l||!b1||!b2)return false;
	if(b1->device==b2->device)return true;
	if(strcmp(b1->name,b2->name)==0)return true;
	return false;
}

static bool block_name_cmp(list*l,void*d){
	LIST_DATA_DECLARE(b1,l,struct use_block*);
	if(!l||!b1)return false;
	if(strcmp(b1->name,d)==0)return true;
	return false;
}

static bool block_path_cmp(list*l,void*d){
	LIST_DATA_DECLARE(b1,l,struct use_block*);
	if(!l||!b1)return false;
	if(strcmp(b1->path,d)==0)return true;
	return false;
}

static bool block_name_sorter(list*l1,list*l2){
	LIST_DATA_DECLARE(b1,l1,struct use_block*);
	LIST_DATA_DECLARE(b2,l2,struct use_block*);
	char*s1=b1->name,*s2=b2->name;
	for(size_t i=0;;i++){
		if(s1[i]==0||s2[i]==0)return false;
		if(s1[i]!=s2[i])return s1[i]>s2[i];
	}
	return false;
}

int gadget_block_delete_name(const char*name){
	struct use_block*b=gadget_lookup_block_by_name(name);
	return b?list_obj_del_data(&gadget_blocks,b,list_default_free):-1;
}

int gadget_block_delete_path(const char*path){
	if(!path||strncmp(path,"/dev/",5)!=0)return -1;
	return gadget_block_delete_name(path+5);
}

int gadget_start(){
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
			if(write_file(gadget_fd,udc,"UDC")<=0)
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

int gadget_stop(){
	DEBUG("stopping gadget\n");
	write_file(gadget_fd,"\n","UDC");
	return 0;
}

static void gadget_add_mass_storage(int b,struct use_block*blk){
	char func[PATH_MAX];
	memset(func,0,sizeof(func));
	snprintf(
		func,sizeof(func)-1,
		"functions/mass_storage.%s",
		blk->name
	);
	mkdirat(gadget_fd,func,0755);
	write_file(gadget_fd,blk->path,"%s/lun.0/file",func);
	symlinkat(func,b,blk->name);
}

struct use_block*gadget_lookup_block_by_name(const char*name){
	list*l=list_search_one(gadget_blocks,block_name_cmp,(void*)name);
	return l?LIST_DATA(l,struct use_block*):NULL;
}

struct use_block*gadget_lookup_block_by_path(const char*path){
	list*l=list_search_one(gadget_blocks,block_path_cmp,(void*)path);
	return l?LIST_DATA(l,struct use_block*):NULL;
}

static int gadget_add_target(struct use_block*blk){
	int d=-1,fd;
	static int lun=0;
	char lun_path[PATH_MAX]={0};
	char blk_path[PATH_MAX]={0};
	if(!blk){
		lun=0;
		return 0;
	}
	snprintf(blk_path,sizeof(blk_path)-1,PATH_TARGET_CORE"/%s",blk->name);
	snprintf(lun_path,sizeof(lun_path)-1,PATH_TPGT"/lun/lun_%d",lun);
	fd=open(blk->path,O_EXCL|O_NDELAY|O_RDWR);
	if(fd<0)return ERROR(-1,"cannot open block");
	close(fd);
	mkdir(blk_path,0755);
	if((d=open(blk_path,O_DIR))>0){
		writef_file(d,"control","udev_path=%s\n",blk->path);
		write_file(d,"1\n","enable");
		write_file(d,blk->name,"wwn/product_id");
		write_file(d,"Qualcomm","wwn/vendor_id");
		close(d);
	}else return ERROR(-1,"open %s failed",blk_path);
	mkdir(lun_path,0755);
	strcat(lun_path,"/virtual_scsi_port");
	if(symlink(blk_path,lun_path)!=0)
		return ERROR(-1,"add new lun failed");
	blk->lun=lun;
	lun++;
	return 0;
}

static int gadget_add_fastboot(int b){
	#define FUNC_NAME "fastboot"
	#define FUNC_PATH "functions/ffs."FUNC_NAME
	#define FUNC_MNT "/dev/ffs."FUNC_NAME
	mkdirat(gadget_fd,FUNC_PATH,0755);
	mkdir(FUNC_MNT,0755);
	if(mount(FUNC_NAME,FUNC_MNT,"functionfs",0,NULL)<0)
		return ERROR(-1,"mount ffs for fastboot failed");
	if(fastboot_init(FUNC_MNT)<0)
		return -1;
	symlinkat(FUNC_PATH,b,FUNC_NAME);
	return 0;
}

static struct use_block*add_block(const char*name){
	int fd;
	bool r=true;
	struct stat st;
	struct use_block blk,*b;
	memset(&blk,0,sizeof(blk));
	strncpy(blk.name,name,sizeof(blk.name)-1);
	snprintf(blk.path,sizeof(blk.path)-1,PATH_DEV"/%s",name);
	if(stat(blk.path,&st)!=0)EPRET(ENOTBLK);
	if(!S_ISBLK(st.st_mode))EPRET(ENOTBLK);
	if((fd=open(blk.path,O_RDONLY))<0)return NULL;
	if(ioctl(fd,BLKGETSIZE64,&blk.size)!=0)r=false;
	if(ioctl(fd,BLKPBSZGET,&blk.sector)!=0)r=false;
	close(fd);
	if(!r)EPRET(errno?:ENOTTY);
	if(blk.size<=0||blk.sector<=0)EPRET(ENOMEDIUM);
	if(list_search_one(gadget_blocks,block_cmp,&blk))EPRET(EEXIST);
	blk.device=st.st_rdev,blk.lun=-1;
	if(!(b=s_memdup(&blk,sizeof(blk))))EPRET(ENOMEM);
	if(list_obj_add_new(&gadget_blocks,b)!=0){
		free(b);
		EPRET(ENOMEM);
	}
	return b;
}

int gadget_block_add_name(const char*name){
	struct use_block*b=gadget_lookup_block_by_name(name);
	if(!b&&!(b=add_block(name)))return -1;
	if(gadget_mode==MODE_TARGET_CORE)
		gadget_add_target(b);
	return 0;
}

int gadget_block_add_path(const char*path){
	if(!path||strncmp(path,"/dev/",5)!=0)return -1;
	return gadget_block_add_name(path+5);
}

int gadget_scan_blocks(){
	DIR*bs=NULL;
	char ss[64];
	struct dirent*d;
	int blk=0,e=0,s=-1;
	struct use_block*b;
	if(!(s=open(PATH_BLOCK,O_DIR))||!(bs=fdopendir(s)))
		EGOTO(-1,"open "PATH_BLOCK" failed");
	while((d=readdir(bs))){
		if(d->d_name[0]=='.'||d->d_type!=DT_LNK)continue;
		if(strncmp(d->d_name,"md",2)==0)continue;
		if(strncmp(d->d_name,"dm-",3)==0)continue;
		if(strncmp(d->d_name,"ram",3)==0)continue;
		if(strncmp(d->d_name,"zram",4)==0)continue;
		//if(strncmp(d->d_name,"loop",4)==0)continue;
		if(read_file(s,ss,sizeof(ss),"%s/size",d->d_name)<=0)continue;
		if(atol(ss)<=0)continue;
		if(!(b=add_block(d->d_name))){
			if(errno!=EEXIST)ERROR(-1,"add block %s failed",d->d_name);
			continue;
		}
		DEBUG(
			"found block %s(%d:%d) size %zu (%s)\n",
			b->name,major(b->device),minor(b->device),
			b->size,format_size(ss,sizeof(ss),b->size,1,0)
		);
		blk++;
	}
	if(blk<=0)EGOTO(-1,"no any block devices found");
	list_sort(gadget_blocks,block_name_sorter);
	DEBUG("found %d blocks\n",blk);
	fail:closedir(bs);
	return e;
}

static int gadget_add_blocks(int b){
	list*l=NULL;
	bool tcm=true;
	int blk=0,e=0;
	errno=0;
	if(gadget_mode==MODE_TARGET_CORE||gadget_mode==MODE_AUTO){
		if(mkdirat(gadget_fd,"functions/tcm.target",0755)<0)tcm=false;
		if(mkdir(PATH_TARGET_CORE,0755)<0)tcm=false;
		if(mkdir(PATH_FABRIC,0755)<0)tcm=false;
		if(mkdir(PATH_NAA,0755)<0)tcm=false;
		if(mkdir(PATH_TPGT,0755)<0)tcm=false;
	}
	if(!tcm){
		if(gadget_mode==MODE_AUTO){
			DEBUG("tcm failed, fallback to mass_storage\n");
			gadget_mode=MODE_MASS_STORAGE;
		}else if(gadget_mode==MODE_TARGET_CORE)return ERROR(-1,"tcm failed");
		else return ERROR(-1,"unknown mass storage gadget_mode");
	}else if(gadget_mode==MODE_AUTO)gadget_mode=MODE_TARGET_CORE;
	else if(gadget_mode==MODE_MASS_STORAGE)tcm=false;
	if((l=list_first(gadget_blocks)))do{
		LIST_DATA_DECLARE(k,l,struct use_block*);
		DEBUG("add block %s\n",k->path);
		switch(gadget_mode){
			case MODE_TARGET_CORE:gadget_add_target(k);break;
			case MODE_MASS_STORAGE:gadget_add_mass_storage(b,k);break;
			default:continue;
		}
		blk++;
	}while((l=l->next));
	if(blk>0)DEBUG("added %d gadget_blocks\n",blk);
	else ERROR(-1,"no any block devices added");
	if(gadget_mode==MODE_TARGET_CORE){
		write_file(AT_FDCWD,TARGET_NAA"\n",PATH_TPGT"/nexus");
		write_file(AT_FDCWD,"1\n",PATH_TPGT"/enable");
		symlinkat("functions/tcm.target",b,"target");
	}
	return e;
}

int gadget_init(void){
	int c=-1,e=0,b=-1;
	if((c=open(PATH_GADGET,O_DIR))<0)
		return ERROR(-1,"open gadget configfs failed");
	DEBUG("init usb gadget\n");
	mkdirat(c,GADGET_NAME,0755);
	if((gadget_fd=openat(c,GADGET_NAME,O_DIR))<0)
		EGOTO(-1,"create gadget folder failed");
	fchdir(gadget_fd);
	writef_file(gadget_fd,"idVendor","0x%04X\n",gadget_config.vendor_id);
	writef_file(gadget_fd,"idProduct","0x%04X\n",gadget_config.product_id);
	writef_file(gadget_fd,"bcdUSB","0x%04X\n",gadget_config.bcd_usb);
	mkdirat(gadget_fd,"strings/0x409",0755);
	writef_file(gadget_fd,"strings/0x409/serialnumber","%s\n",gadget_config.serialnumber);
	writef_file(gadget_fd,"strings/0x409/manufacturer","%s\n",gadget_config.manufacturer);
	writef_file(gadget_fd,"strings/0x409/product","%s\n",gadget_config.product);
	write_file(gadget_fd,"1\n","os_desc/use");
	write_file(gadget_fd,"0x1\n","os_desc/b_vendor_code");
	write_file(gadget_fd,"MSFT100\n","os_desc/qw_sign");
	if(mkdirat(gadget_fd,"configs/b.1",0755)<0)
		EGOTO(-1,"add gadget config failed");
	symlinkat("configs/b.1",gadget_fd,"os_desc/b.1");
	if((b=openat(gadget_fd,"configs/b.1",O_DIR))<0)
		EGOTO(-1,"open gadget config failed");
	mkdirat(b,"strings/0x409",0755);
	writef_file(b,"strings/0x409/configuration","%s\n","FASTBOOT");
	gadget_add_target(NULL);
	gadget_add_fastboot(b);
	gadget_add_blocks(b);
	if(gadget_start()!=0)EGOTO(-1,"start gadget failed");
	chdir("/");
	DEBUG("gadget initialized\n");
	fail:
	if(b>=0)close(b);
	if(b>=0)close(b);
	if(c>=0)close(c);
	if(e!=0){
		if(gadget_fd>=0)close(gadget_fd);
		gadget_fd=-1;
	}
	return e;
}
