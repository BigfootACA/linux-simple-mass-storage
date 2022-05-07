ARCH              = arm64
TARGET            = aarch64-linux-gnu
CROSS_COMPILE     = $(TARGET)-
CC                = $(CROSS_COMPILE)gcc
KBUILD_BUILD_USER = bigfoot
KBUILD_BUILD_HOST = classfun.cn
SYSROOT           = $(PWD)/sysroot
LIBDIR            = $(SYSROOT)/lib
X_CCFLAGS         = -nostdinc -nolibc -nostartfiles -nostdlib -Os
X_CFLAGS          = -Werror -Wall -Wextra -isystem $(SYSROOT)/include
X_LDFLAGS         = -static -s -flto
X_LIBS            = $(LIBDIR)/crt1.o $(LIBDIR)/libc.a $(LIBDIR)/crti.o $(LIBDIR)/crtn.o -lgcc
export ARCH CROSS_COMPILE
all: Image.gz LinuxSimpleMassStorage.efi
src/init.c: src/config.h sysroot/include
src/init.o: src/init.c
	$(CC) $(X_CCFLAGS) $(X_CFLAGS) -c $< -o $@
init: src/init.o sysroot/lib/libc.a
	$(CC) $(X_CCFLAGS) $(X_LDFLAGS) $< $(X_LIBS) -o $@
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
.PHONY: clean clean-bin clean-musl clean-kernel clean-sysroot all
