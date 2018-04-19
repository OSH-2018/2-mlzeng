scp init.c station:/home/zml/exp2/mini-os/rootfs/
ssh -t station "cd /home/zml/exp2/mini-os/rootfs/; gcc -static -o init init.c; find . | cpio -oHnewc | gzip > ../initramfs.gz"
scp station:/home/zml/exp2/mini-os/initramfs.gz ./
scp station:/home/zml/exp2/linux-4.16.2/arch/x86_64/boot/bzImage ./
ssh -t station "cd /home/zml/exp2/; ./run.sh"
