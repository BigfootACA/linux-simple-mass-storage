#include"config.h"
#include"init.h"
#include"list.h"
#include<linux/fs.h>
#include<sys/mount.h>

static void target_tptg_clean(int sfd){
	DIR*d;
	int fd,dfd;
	struct dirent*e;
	write_file(sfd,"0\n","enable");
	if((dfd=openat(sfd,"lun",O_DIR))<0)return;
	if((d=fdopendir(dfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			if(strncmp(e->d_name,"lun_",4)!=0)continue;
			if((fd=openat(dfd,e->d_name,O_DIR))<0)continue;
			unlinkat(fd,"virtual_scsi_port",0);
			close(fd);
			unlinkat(dfd,e->d_name,AT_REMOVEDIR);
		}
		closedir(d);
	}
	close(dfd);
}

static void target_naa_clean(int sfd){
	DIR*d;
	int fd;
	struct dirent*e;
	if((d=fdopendir(sfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			if(strncmp(e->d_name,"tpgt_",5)!=0)continue;
			if((fd=openat(sfd,e->d_name,O_DIR))<0)continue;
			target_tptg_clean(fd);
			close(fd);
			unlinkat(sfd,e->d_name,AT_REMOVEDIR);
		}
		closedir(d);
	}
}

static void target_config_clean(int sfd){
	DIR*d;
	int fd;
	struct dirent*e;
	if((d=fdopendir(sfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			if(strncmp(e->d_name,"naa.",4)!=0)continue;
			if((fd=openat(sfd,e->d_name,O_DIR))<0)continue;
			target_naa_clean(fd);
			close(fd);
			unlinkat(sfd,e->d_name,AT_REMOVEDIR);
		}
		closedir(d);
	}
}

static void target_core_clean(int sfd){
	DIR*d;
	int fd;
	struct dirent*e;
	if((d=fdopendir(sfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			if(strcmp(e->d_name,"alua")==0)continue;
			if((fd=openat(sfd,e->d_name,O_DIR))<0)continue;
			remove_folders(fd,AT_REMOVEDIR);
			close(fd);
			unlinkat(sfd,e->d_name,AT_REMOVEDIR);
		}
		closedir(d);
	}
}

static void target_clean(){
	DIR*d;
	int fd,dfd;
	struct dirent*e;
	if((dfd=open(PATH_TARGET,O_DIR))<0)return;
	if((d=fdopendir(dfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			if((fd=openat(dfd,e->d_name,O_DIR))<0)continue;
			if(strcmp(e->d_name,"core")!=0){
				target_config_clean(fd);
				unlinkat(dfd,e->d_name,AT_REMOVEDIR);
			}else target_core_clean(fd);
			close(fd);
		}
		closedir(d);
	}
	close(dfd);
}

static void ffs_clean(const char*name){
	struct mount_item**ms,*m;
	if(!(ms=read_proc_mounts()))return;
	for(size_t i=0;(m=ms[i]);i++){
		if(!m->type||!m->target||!m->source)continue;
		if(strcmp(m->type,"functionfs")!=0)continue;
		if(strcmp(m->source,name)!=0)continue;
		umount(m->target);
		unlink(m->target);
	}
	free_mounts(ms);
}

static void string_clean(int sfd){
	int dfd;
	if((dfd=openat(sfd,"strings",O_DIR))<0)return;
	remove_folders(dfd,AT_REMOVEDIR);
	close(dfd);
}

static void gadget_config_clean(int sfd){
	DIR*d;
	struct dirent*e;
	if((d=fdopendir(sfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_LNK)continue;
			string_clean(sfd);
			unlinkat(sfd,e->d_name,0);
		}
		closedir(d);
	}
}

static void gadget_configs_clean(int sfd){
	DIR*d;
	int fd,dfd;
	struct dirent*e;
	if((dfd=openat(sfd,"configs",O_DIR))<0)return;
	if((d=fdopendir(dfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			if((fd=openat(dfd,e->d_name,O_DIR))<0)continue;
			gadget_config_clean(fd);
			close(fd);
			unlinkat(dfd,e->d_name,AT_REMOVEDIR);
		}
		closedir(d);
	}
	close(dfd);
}

static void gadget_functions_clean(int sfd){
	DIR*d;
	int dfd;
	struct dirent*e;
	char buff[BUFSIZ],*type,*name,*p;
	if((dfd=openat(sfd,"functions",O_DIR))<0)return;
	if((d=fdopendir(dfd))){
		while((e=readdir(d))){
			if(e->d_name[0]=='.')continue;
			if(e->d_type!=DT_DIR)continue;
			memset(buff,0,sizeof(buff));
			strncpy(buff,e->d_name,sizeof(buff)-1);
			if(!(p=strchr(buff,'.')))continue;
			*p=0,type=buff,name=p+1;
			if(!type[0]||!name[0])continue;
			if(strcmp(type,"ffs")==0){
				if(strcmp(name,"fastboot")==0)
					fastboot_stop();
				ffs_clean(name);
			}
			if(strcmp(type,"tcm")==0)
				target_clean();
			unlinkat(dfd,e->d_name,AT_REMOVEDIR);
		}
		closedir(d);
	}
	close(dfd);
}

static void gadget_clean(int sfd){
	write_file(sfd,"\n","UDC");
	string_clean(sfd);
	gadget_configs_clean(sfd);
	gadget_functions_clean(sfd);
}

int gadget_cleanup(const char*name){
	DIR*d;
	int fd,dfd;
	struct dirent*e;
	if((dfd=open(PATH_GADGET,O_DIR))>=0){
		if(name&&name[0]){
			if((fd=openat(dfd,name,O_DIR))<0)return -1;
			gadget_clean(fd);
			close(fd);
			unlinkat(dfd,name,AT_REMOVEDIR);
		}else if((d=fdopendir(dfd))){
			while((e=readdir(d))){
				if(e->d_name[0]=='.')continue;
				if(e->d_type!=DT_DIR)continue;
				if((fd=openat(dfd,e->d_name,O_DIR))<0)continue;
				gadget_clean(fd);
				close(fd);
				unlinkat(dfd,e->d_name,AT_REMOVEDIR);
			}
			closedir(d);
		}
		close(dfd);
	}
	return 0;
}
