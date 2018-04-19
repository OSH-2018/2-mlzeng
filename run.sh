#!/bin/sh
KERNEL="bzImage"
INITRD="initramfs.gz"
APPEND="console=ttyS0"

qemu-system-x86_64 \
	-append "$APPEND" \
	-initrd "$INITRD" \
	-kernel "$KERNEL" \
	-nographic
