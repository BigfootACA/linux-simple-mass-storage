INIT_LINKS = \
	src/main.o \
	src/init.a \
	external/libblkid/libblkid.a \
	sysroot/lib/libc.a
INIT_OBJS = \
	src/cleanup.o \
	src/debug.o \
	src/devfs.o \
	src/display.o \
	src/gadget.o \
	src/init.o \
	src/lib.o \
	src/list.o \
	src/fastboot/blocks.o \
	src/fastboot/command.o \
	src/fastboot/constructor.o \
	src/fastboot/fastboot.o \
	src/fastboot/sparse.o \
	src/fastboot/variable.o \
	src/fastboot/commands/buffer.o \
	src/fastboot/commands/file.o \
	src/fastboot/commands/flash.o \
	src/fastboot/commands/gadget.o \
	src/fastboot/commands/utils.o \
	src/fastboot/commands/variable.o \
	src/fastboot/commands/reboot.o
BLKID_OBJS = \
	external/libblkid/read.o \
	external/libblkid/devname.o \
	external/libblkid/version.o \
	external/libblkid/getsize.o \
	external/libblkid/tag.o \
	external/libblkid/md5.o \
	external/libblkid/encode.o \
	external/libblkid/superblocks/zonefs.o \
	external/libblkid/superblocks/vdo.o \
	external/libblkid/superblocks/jfs.o \
	external/libblkid/superblocks/adaptec_raid.o \
	external/libblkid/superblocks/iso9660.o \
	external/libblkid/superblocks/f2fs.o \
	external/libblkid/superblocks/ufs.o \
	external/libblkid/superblocks/befs.o \
	external/libblkid/superblocks/btrfs.o \
	external/libblkid/superblocks/bcache.o \
	external/libblkid/superblocks/vmfs.o \
	external/libblkid/superblocks/luks.o \
	external/libblkid/superblocks/lvm.o \
	external/libblkid/superblocks/vfat.o \
	external/libblkid/superblocks/drbdproxy_datalog.o \
	external/libblkid/superblocks/hfs.o \
	external/libblkid/superblocks/via_raid.o \
	external/libblkid/superblocks/highpoint_raid.o \
	external/libblkid/superblocks/erofs.o \
	external/libblkid/superblocks/cramfs.o \
	external/libblkid/superblocks/bfs.o \
	external/libblkid/superblocks/minix.o \
	external/libblkid/superblocks/ubifs.o \
	external/libblkid/superblocks/promise_raid.o \
	external/libblkid/superblocks/ocfs.o \
	external/libblkid/superblocks/stratis.o \
	external/libblkid/superblocks/nilfs.o \
	external/libblkid/superblocks/vxfs.o \
	external/libblkid/superblocks/drbdmanage.o \
	external/libblkid/superblocks/nvidia_raid.o \
	external/libblkid/superblocks/silicon_raid.o \
	external/libblkid/superblocks/superblocks.o \
	external/libblkid/superblocks/gfs.o \
	external/libblkid/superblocks/hpfs.o \
	external/libblkid/superblocks/udf.o \
	external/libblkid/superblocks/mpool.o \
	external/libblkid/superblocks/isw_raid.o \
	external/libblkid/superblocks/exfs.o \
	external/libblkid/superblocks/romfs.o \
	external/libblkid/superblocks/jmicron_raid.o \
	external/libblkid/superblocks/lsi_raid.o \
	external/libblkid/superblocks/refs.o \
	external/libblkid/superblocks/bitlocker.o \
	external/libblkid/superblocks/xfs.o \
	external/libblkid/superblocks/exfat.o \
	external/libblkid/superblocks/linux_raid.o \
	external/libblkid/superblocks/squashfs.o \
	external/libblkid/superblocks/sysv.o \
	external/libblkid/superblocks/reiserfs.o \
	external/libblkid/superblocks/ext.o \
	external/libblkid/superblocks/swap.o \
	external/libblkid/superblocks/netware.o \
	external/libblkid/superblocks/bluestore.o \
	external/libblkid/superblocks/ddf_raid.o \
	external/libblkid/superblocks/ubi.o \
	external/libblkid/superblocks/apfs.o \
	external/libblkid/superblocks/ntfs.o \
	external/libblkid/superblocks/drbd.o \
	external/libblkid/superblocks/zfs.o \
	external/libblkid/resolve.o \
	external/libblkid/verify.o \
	external/libblkid/dev.o \
	external/libblkid/probe.o \
	external/libblkid/config.o \
	external/libblkid/evaluate.o \
	external/libblkid/save.o \
	external/libblkid/topology/sysfs.o \
	external/libblkid/topology/lvm.o \
	external/libblkid/topology/ioctl.o \
	external/libblkid/topology/md.o \
	external/libblkid/topology/evms.o \
	external/libblkid/topology/topology.o \
	external/libblkid/topology/dm.o \
	external/libblkid/cache.o \
	external/libblkid/partitions/sun.o \
	external/libblkid/partitions/ultrix.o \
	external/libblkid/partitions/minix.o \
	external/libblkid/partitions/bsd.o \
	external/libblkid/partitions/solaris_x86.o \
	external/libblkid/partitions/gpt.o \
	external/libblkid/partitions/sgi.o \
	external/libblkid/partitions/partitions.o \
	external/libblkid/partitions/aix.o \
	external/libblkid/partitions/unixware.o \
	external/libblkid/partitions/atari.o \
	external/libblkid/partitions/dos.o \
	external/libblkid/partitions/mac.o \
	external/libblkid/devno.o \
	external/libblkid/lib/fileutils.o \
	external/libblkid/lib/mangle.o \
	external/libblkid/lib/sysfs.o \
	external/libblkid/lib/jsonwrt.o \
	external/libblkid/lib/path.o \
	external/libblkid/lib/crc32.o \
	external/libblkid/lib/match.o \
	external/libblkid/lib/encode.o \
	external/libblkid/lib/monotonic.o \
	external/libblkid/lib/strutils.o \
	external/libblkid/lib/linux_version.o \
	external/libblkid/lib/mbsalign.o \
	external/libblkid/lib/crc32c.o \
	external/libblkid/lib/blkdev.o \
	external/libblkid/lib/buffer.o \
	external/libblkid/lib/loopdev.o \
	external/libblkid/lib/canonicalize.o
external/libblkid/libblkid.a: $(BLKID_OBJS)
src/init.a: $(INIT_OBJS)
src/init.h: src/config.h src/list.h sysroot/include
src/init.c: src/init.h sysroot/include
src/lib.c: src/init.h sysroot/include
src/debug.c: src/init.h sysroot/include
src/devfs.c: src/init.h sysroot/include
src/cleanup.c: src/init.h sysroot/include
src/display.c: external/nanosvg.h external/nanosvgrast.h external/stb_image.h external/stb_image_resize.h src/init.h sysroot/include
src/gadget.c: src/init.h sysroot/include
src/list.c: src/list.h src/init.h sysroot/include
src/fastboot/blocks.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/command.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/constructor.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/descriptors.h: src/init.h sysroot/include
src/fastboot/fastboot.c: src/fastboot/descriptors.h src/fastboot/fastboot.h sysroot/include
src/fastboot/fastboot.h: src/init.h sysroot/include
src/fastboot/sparse.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/variable.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/buffer.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/file.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/flash.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/gadget.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/reboot.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/utils.c: src/fastboot/fastboot.h sysroot/include
src/fastboot/commands/variable.c: src/fastboot/fastboot.h sysroot/include
external/libblkid/read.c: sysroot/include
external/libblkid/devname.c: sysroot/include
external/libblkid/version.c: sysroot/include
external/libblkid/getsize.c: sysroot/include
external/libblkid/tag.c: sysroot/include
external/libblkid/md5.c: sysroot/include
external/libblkid/encode.c: sysroot/include
external/libblkid/superblocks/zonefs.c: sysroot/include
external/libblkid/superblocks/vdo.c: sysroot/include
external/libblkid/superblocks/jfs.c: sysroot/include
external/libblkid/superblocks/adaptec_raid.c: sysroot/include
external/libblkid/superblocks/iso9660.c: sysroot/include
external/libblkid/superblocks/f2fs.c: sysroot/include
external/libblkid/superblocks/ufs.c: sysroot/include
external/libblkid/superblocks/befs.c: sysroot/include
external/libblkid/superblocks/btrfs.c: sysroot/include
external/libblkid/superblocks/bcache.c: sysroot/include
external/libblkid/superblocks/vmfs.c: sysroot/include
external/libblkid/superblocks/luks.c: sysroot/include
external/libblkid/superblocks/lvm.c: sysroot/include
external/libblkid/superblocks/vfat.c: sysroot/include
external/libblkid/superblocks/drbdproxy_datalog.c: sysroot/include
external/libblkid/superblocks/hfs.c: sysroot/include
external/libblkid/superblocks/via_raid.c: sysroot/include
external/libblkid/superblocks/highpoint_raid.c: sysroot/include
external/libblkid/superblocks/erofs.c: sysroot/include
external/libblkid/superblocks/cramfs.c: sysroot/include
external/libblkid/superblocks/bfs.c: sysroot/include
external/libblkid/superblocks/minix.c: sysroot/include
external/libblkid/superblocks/ubifs.c: sysroot/include
external/libblkid/superblocks/promise_raid.c: sysroot/include
external/libblkid/superblocks/ocfs.c: sysroot/include
external/libblkid/superblocks/stratis.c: sysroot/include
external/libblkid/superblocks/nilfs.c: sysroot/include
external/libblkid/superblocks/vxfs.c: sysroot/include
external/libblkid/superblocks/drbdmanage.c: sysroot/include
external/libblkid/superblocks/nvidia_raid.c: sysroot/include
external/libblkid/superblocks/silicon_raid.c: sysroot/include
external/libblkid/superblocks/superblocks.c: sysroot/include
external/libblkid/superblocks/gfs.c: sysroot/include
external/libblkid/superblocks/hpfs.c: sysroot/include
external/libblkid/superblocks/udf.c: sysroot/include
external/libblkid/superblocks/mpool.c: sysroot/include
external/libblkid/superblocks/isw_raid.c: sysroot/include
external/libblkid/superblocks/exfs.c: sysroot/include
external/libblkid/superblocks/romfs.c: sysroot/include
external/libblkid/superblocks/jmicron_raid.c: sysroot/include
external/libblkid/superblocks/lsi_raid.c: sysroot/include
external/libblkid/superblocks/refs.c: sysroot/include
external/libblkid/superblocks/bitlocker.c: sysroot/include
external/libblkid/superblocks/xfs.c: sysroot/include
external/libblkid/superblocks/exfat.c: sysroot/include
external/libblkid/superblocks/linux_raid.c: sysroot/include
external/libblkid/superblocks/squashfs.c: sysroot/include
external/libblkid/superblocks/sysv.c: sysroot/include
external/libblkid/superblocks/reiserfs.c: sysroot/include
external/libblkid/superblocks/ext.c: sysroot/include
external/libblkid/superblocks/swap.c: sysroot/include
external/libblkid/superblocks/netware.c: sysroot/include
external/libblkid/superblocks/bluestore.c: sysroot/include
external/libblkid/superblocks/ddf_raid.c: sysroot/include
external/libblkid/superblocks/ubi.c: sysroot/include
external/libblkid/superblocks/apfs.c: sysroot/include
external/libblkid/superblocks/ntfs.c: sysroot/include
external/libblkid/superblocks/drbd.c: sysroot/include
external/libblkid/superblocks/zfs.c: sysroot/include
external/libblkid/resolve.c: sysroot/include
external/libblkid/verify.c: sysroot/include
external/libblkid/dev.c: sysroot/include
external/libblkid/probe.c: sysroot/include
external/libblkid/config.c: sysroot/include
external/libblkid/evaluate.c: sysroot/include
external/libblkid/save.c: sysroot/include
external/libblkid/topology/sysfs.c: sysroot/include
external/libblkid/topology/lvm.c: sysroot/include
external/libblkid/topology/ioctl.c: sysroot/include
external/libblkid/topology/md.c: sysroot/include
external/libblkid/topology/evms.c: sysroot/include
external/libblkid/topology/topology.c: sysroot/include
external/libblkid/topology/dm.c: sysroot/include
external/libblkid/cache.c: sysroot/include
external/libblkid/partitions/sun.c: sysroot/include
external/libblkid/partitions/ultrix.c: sysroot/include
external/libblkid/partitions/minix.c: sysroot/include
external/libblkid/partitions/bsd.c: sysroot/include
external/libblkid/partitions/solaris_x86.c: sysroot/include
external/libblkid/partitions/gpt.c: sysroot/include
external/libblkid/partitions/sgi.c: sysroot/include
external/libblkid/partitions/partitions.c: sysroot/include
external/libblkid/partitions/aix.c: sysroot/include
external/libblkid/partitions/unixware.c: sysroot/include
external/libblkid/partitions/atari.c: sysroot/include
external/libblkid/partitions/dos.c: sysroot/include
external/libblkid/partitions/mac.c: sysroot/include
external/libblkid/devno.c: sysroot/include
external/libblkid/lib/fileutils.c: sysroot/include
external/libblkid/lib/mangle.c: sysroot/include
external/libblkid/lib/sysfs.c: sysroot/include
external/libblkid/lib/jsonwrt.c: sysroot/include
external/libblkid/lib/path.c: sysroot/include
external/libblkid/lib/crc32.c: sysroot/include
external/libblkid/lib/match.c: sysroot/include
external/libblkid/lib/encode.c: sysroot/include
external/libblkid/lib/monotonic.c: sysroot/include
external/libblkid/lib/strutils.c: sysroot/include
external/libblkid/lib/linux_version.c: sysroot/include
external/libblkid/lib/mbsalign.c: sysroot/include
external/libblkid/lib/crc32c.c: sysroot/include
external/libblkid/lib/blkdev.c: sysroot/include
external/libblkid/lib/buffer.c: sysroot/include
external/libblkid/lib/loopdev.c: sysroot/include
external/libblkid/lib/canonicalize.c: sysroot/include
