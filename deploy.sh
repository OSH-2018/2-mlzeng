scp init.c station:/home/zml/exp2/mini-os/rootfs/
ssh -t station "cd /home/zml/exp2/mini-os/rootfs/; gcc -static -o init init.c; find . | cpio -oHnewc | gzip > ../initramfs.gz"
ssh -t station "cd /home/zml/exp2/; ./run.sh"
