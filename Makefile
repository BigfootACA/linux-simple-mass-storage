ARCH              = arm64
TARGET            = aarch64-linux-gnu
CROSS_COMPILE     = $(TARGET)-
CC                = $(CROSS_COMPILE)gcc
AR                = $(CROSS_COMPILE)ar
STRIP             = $(CROSS_COMPILE)strip
RANLIB            = $(CROSS_COMPILE)ranlib
KBUILD_BUILD_USER = bigfoot
KBUILD_BUILD_HOST = classfun.cn
SYSROOT           = $(PWD)/sysroot
LIBDIR            = $(SYSROOT)/lib
X_CCFLAGS         = -Os -g -nostdinc -nolibc -nostartfiles -nostdlib
X_CFLAGS          = -D_GNU_SOURCE -Werror -Wall -Wextra -Wno-format-truncation -Iexternal -Isrc -isystem $(SYSROOT)/include
X_LDFLAGS         = -flto -static
X_LIBS            = $(LIBDIR)/crt1.o $(LIBDIR)/libc.a $(LIBDIR)/crti.o $(LIBDIR)/crtn.o -lgcc
export ARCH CROSS_COMPILE
all: Image.gz LinuxSimpleMassStorage.efi
include deps.mk
%.a:
	$(AR) cr $@ $^
	$(RANLIB) $@
src/%.o: src/%.c
	$(CC) $(X_CCFLAGS) $(X_CFLAGS) -c $< -o $@
external/libblkid/%.o: external/libblkid/%.c
	$(CC) $(X_CCFLAGS) $(X_CFLAGS) -Iexternal/libblkid -c $< -o $@
init_debug: $(INIT_LINKS)
	$(CC) $(X_CCFLAGS) $(X_LDFLAGS) -Wl,--start-group $^ -Wl,--end-group $(X_LIBS) -o $@
init: init_debug
	$(STRIP) -o $@ $<
musl/config.mak: musl/configure
	cd musl;./configure --target=$(TARGET) --prefix=/
musl/lib/libc.a: musl/config.mak
	$(MAKE) -C musl
sysroot/include: sysroot/lib/libc.a
sysroot/lib/libc.a: musl/lib/libc.a musl/config.mak kernel/Makefile
	$(MAKE) -C musl DESTDIR="$(SYSROOT)" install
	$(MAKE) -C kernel INSTALL_HDR_PATH="$(SYSROOT)" headers_install
initramfs.cpio: init splash-started.svg splash-starting.svg
	ls -1 $^ | cpio -o -H newc > $@
kernel/.config: configs/$(CONFIG).config kernel/Makefile
	cp $< kernel/arch/arm64/configs/kernel_defconfig
	$(MAKE) -C kernel kernel_defconfig
	rm kernel/arch/arm64/configs/kernel_defconfig
kernel/arch/arm64/boot/Image: initramfs.cpio kernel/.config kernel/Makefile
	$(MAKE) -C kernel Image
musl/configure: .stamp-submodule
kernel/Makefile: .stamp-submodule
.stamp-submodule: .gitmodules
	git submodule init
	git submodule sync
	git submodule update --depth 1 --jobs 2
	touch .stamp-submodule
kernel-menuconfig: configs/$(CONFIG).config kernel/Makefile
	$(MAKE) -C kernel menuconfig
qemu-run: Image.gz
	qemu-system-aarch64 \
		-m 1G \
		-display none \
		-machine virt \
		-cpu cortex-a57 \
		-kernel Image.gz \
		-chardev stdio,id=char0 \
		-chardev pty,id=char1 \
		-netdev bridge,id=eth0,br=virbr0 \
		-serial chardev:char0 \
		-drive file=1.img,format=raw,if=none,id=blk0 \
		-device pci-serial,chardev=char1 \
		-device e1000e,netdev=eth0 \
		-device nvme,drive=blk0,serial=INTERNAL
Image: kernel/arch/arm64/boot/Image
	cp $< $@
Image.gz: Image
	gzip < $< > $@
LinuxSimpleMassStorage.efi: Image LinuxSimpleMassStorage.inf
	cp $< $@
clean-bin:
	rm -f initramfs.cpio init *.o Image Image.gz
clean-musl:
	$(MAKE) -C musl clean
clean-kernel:
	$(MAKE) -C kernel clean
clean-sysroot:
	rm -rf sysroot
clean: clean-bin clean-musl clean-kernel clean-sysroot
.PHONY: clean clean-bin clean-musl clean-kernel clean-sysroot all kernel-menuconfig
