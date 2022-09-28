#include"config.h"
#include"init.h"
#include"list.h"
#include<sys/mount.h>

static void init_fs(void){
	chdir("/");
	mkdir(PATH_DEV,0755);
	mkdir(PATH_SYS,0555);
	mkdir(PATH_PROC,0555);
	#if USE_DEVTMPFS == 0
	xmount("devfs",PATH_DEV,"tmpfs",MS_NOSUID,"mode=755");
	#else
	xmount("devfs",PATH_DEV,"devtmpfs",MS_NOSUID,"mode=755");
	#endif
	xmount("sysfs",PATH_SYS,"sysfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
	#if USE_DEVTMPFS == 0
	init_devfs();
	#endif
	mkdir(PATH_HUGEPAGES,0755);
	mkdir(PATH_MQUEUE,01777);
	mkdir(PATH_SHM,01777);
	mkdir(PATH_TMP,01777);
	init_console();
	mount("proc",PATH_PROC,"proc",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
	xmount("configfs",PATH_CONFIGFS,"configfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
	mount("hugetlbfs",PATH_HUGEPAGES,"hugetlbfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"pagesize=2M");
	mount("mqueue",PATH_MQUEUE,"mqueue",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
	mount("shm",PATH_SHM,"tmpfs",MS_NOSUID|MS_NODEV,"mode=1777");
	mount("tmp",PATH_TMP,"tmpfs",MS_NOSUID|MS_NODEV,"mode=1777");
}

static void check_env(void){
	if(getuid()!=0)FAILURE(1,"Not UID 0");
	if(getgid()!=0)FAILURE(1,"Not GID 0");
}

int init_main(void){
	#if USE_BUSYBOX_ASH == 1 || USE_GDB_SERVER == 1 || USE_USBIPD == 1
	pid_t p;
	#endif
	check_env();
	if(getpid()==1){
		init_fs();
		#if DISPLAY_INIT == 1
		init_fbdev();
		#endif
	}
	#if DISPLAY_IMAGE == 1
	draw_svg_splash(IMG_PRE_PATH);
	#endif
	if(getpid()==1){
		#if USE_BUSYBOX_ASH == 1
		if((p=fork())==0)_exit(execl("/busybox","ash",NULL));
		DEBUG("starting busybox ash as %d\n",p);
		#endif
		#if USE_GDB_SERVER == 1
		if((p=fork())==0)_exit(execl(
			"/gdbserver","gdbserver",
			"--attach",GDB_SERVER_PORT,
			"1",NULL
		));
		DEBUG("starting gdbserver as %d\n",p);
		sleep(3);
		#endif
	}
	gadget_scan_blocks();
	gadget_init();
	#if DISPLAY_IMAGE == 1
	draw_svg_splash(IMG_POST_PATH);
	#endif
	if(getpid()==1){
		#if USE_USBIPD == 1
		if((p=fork())==0)_exit(execl(
			"/usbipd","usbipd",
			"-e","-4",NULL
		));
		DEBUG("starting usbipd as %d\n",p);
		#endif
		while(1)sleep(3600);
	}
	return 0;
}
