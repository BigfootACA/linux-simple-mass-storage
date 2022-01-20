# Simple Mass Storage Kernel

## Build

```bash
sudo apt update
sudo apt install -y build-essential git gcc-aarch64-linux-gnu bc libncurses-dev make cpio gzip
make CONFIG=sdm845-minimal
```

## Note

You need a dtb to boot
