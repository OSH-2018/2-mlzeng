# 实验报告

## 准备实验环境

本次实验使用 ArchLinux 发行版进行。ArchLinux 简洁轻量，软件包非常新，注重实用性。使用其包管理器 pacman 可以很方便的完成实验环境的准备。

### 在 chroot 环境中安装 ArchLinux

由于服务器上的操作系统是 Ubuntu，若要不借助虚拟化技术在上面搭建 ArchLinux 开发环境，可以通过使用 chroot 来实现。（也可以使用比 chroot 完善的 Docker）

#### 下载 bootstrap 镜像

```shell
zml@zml-HP-Z420-Workstation:~ % mkdir archlinux
zml@zml-HP-Z420-Workstation:~ % cd archlinux
zml@zml-HP-Z420-Workstation:~/archlinux % wget http://mirrors.ustc.edu.cn/archlinux/iso/latest/archlinux-bootstrap-2018.04.01-x86_64.tar.gz
```

#### 解压 bootstrap 镜像

```shell
zml@zml-HP-Z420-Workstation:~/archlinux % tar xzf archlinux-bootstrap-2018.04.01-x86_64.tar.gz
```

#### 更改软件源为科大源

```shell
zml@zml-HP-Z420-Workstation:~/archlinux % cat > root.x86_64/etc/pacman.d/mirrorlist
Server = https://mirrors.ustc.edu.cn/archlinux/$repo/os/$arch
```

#### 进入 chroot 环境

```shell
zml@zml-HP-Z420-Workstation:~/archlinux % sudo root.x86_64/bin/arch-chroot root.x86_64
```

执行命令后可以看到 shell 的提示符变成：

```shell
[root@zml-HP-Z420-Workstation /]#
```

此时已经进入了 chroot 环境。

### 使用 pacman 安装实验所需软件包

现在已经在 chroot 里面搭建好了 ArchLinux 的最简环境，里面只有非常少的软件。此时需要安装 ArchLinux 专用的包管理器 pacman，然后就可以用 pacman 在 chroot 环境中方便地安装所需软件了。

#### 禁用 pacman 的硬盘剩余空间检查

由于在 chroot 环境中**已挂载文件系统表** `/etc/mtab` 中没有 `/` 的记录，这会导致 pacman 检查硬盘剩余空间时出错。运行 `df` 命令可以看出没有 rootfs 的记录。

```shell
[root@zml-HP-Z420-Workstation /]# df
Filesystem     1K-blocks  Used Available Use% Mounted on
udev            32916660     0  32916660   0% /dev
shm             32948028     0  32948028   0% /dev/shm
run             32948028     0  32948028   0% /run
tmp             32948028     0  32948028   0% /tmp
tmpfs            6589608  2224   6587384   1% /etc/resolv.conf
```

编辑 `/etc/pacman.conf` 将 checkspace 注释掉即可。

```shell
#CheckSpace
```

#### 初始化 pacman 的密钥环

```shell
[root@zml-HP-Z420-Workstation /]# pacman-key --init
[root@zml-HP-Z420-Workstation /]# pacman-key --populate archlinux
```

#### 更新软件包列表

```shell
[root@zml-HP-Z420-Workstation /]# pacman -Syyu
```

#### 安装基本包组

```shell
[root@zml-HP-Z420-Workstation /]# pacman -S base base-devel
```

#### 安装必要的软件

```shell
[root@zml-HP-Z420-Workstation /]# pacman -S qemu gdb vim bc cpio
```

至此，ArchLinux 实验环境就基本搭建完成了。

## Linux 内核的裁剪

从科大镜像站下载最新的 Linux 内核源码并解压。

```shell
[root@zml-HP-Z420-Workstation exp2]# curl http://mirrors.ustc.edu.cn/kernel.org/linux/kernel/v4.x/linux-4.16.2.tar.xz | tar xJf -
```

进入源代码文件夹，生成配置文件，配置编译选项，并开始编译。

```shell
[root@zml-HP-Z420-Workstation linux-4.16.2]# make allnoconfig
[root@zml-HP-Z420-Workstation linux-4.16.2]# make menuconfig
[root@zml-HP-Z420-Workstation linux-4.16.2]# make -j
```

## 制作根文件系统

### 创建目录

```shell
[root@zml-HP-Z420-Workstation exp2]# mkdir -pv ./mini-os/rootfs/{dev,proc,sys,run}
```

### 创建初始设备节点

```shell
[root@zml-HP-Z420-Workstation exp2]# cd mini-os/rootfs/
[root@zml-HP-Z420-Workstation rootfs]# mknod -m 622 ./dev/console c 5 1
[root@zml-HP-Z420-Workstation rootfs]# mknod -m 666 ./dev/null c 1 3
[root@zml-HP-Z420-Workstation rootfs]# mknod -m 666 ./dev/zero c 1 5
[root@zml-HP-Z420-Workstation rootfs]# mknod -m 666 ./dev/ptmx c 5 2
[root@zml-HP-Z420-Workstation rootfs]# mknod -m 666 ./dev/tty c 5 0
[root@zml-HP-Z420-Workstation rootfs]# mknod -m 666 ./dev/ttyS0 c 4 64
```

## 编译安装 busybox

从官方网站获取最新版 busybox 的源代码并解压。

```shell
[root@zml-HP-Z420-Workstation exp2]# curl https://busybox.net/downloads/busybox-1.28.3.tar.bz2 | tar xjf -
```

进入源代码文件夹，生成配置文件，配置编译选项（编译成不依赖任何动态链接库的静态可执行文件），并开始编译。

```shell
[root@zml-HP-Z420-Workstation exp2]# cd busybox-1.28.3
[root@zml-HP-Z420-Workstation busybox-1.28.3]# make defconfig
[root@zml-HP-Z420-Workstation busybox-1.28.3]# make menuconfig
[root@zml-HP-Z420-Workstation busybox-1.28.3]# make -j
```

将编译好的二进制文件安装到根文件系统中。

```shell
[root@zml-HP-Z420-Workstation busybox-1.28.3]# make CONFIG_PREFIX=../mini-os/rootfs install
```

## 编写 shell 程序

### 让 shell 程序更健壮

#### 处理不良输入

在拆解命令行部分，遇到空格后跳过紧跟其后的所有空格即可。

```c
while (*args[i + 1] && isblank(*args[i + 1]))
    args[i + 1]++;
```

#### 处理错误

##### 读入流发生错误

在 `fgets()` 函数之后立即检查错误。

```c
if (ferror(stdin))
{
    perror("Input stream error!");
    continue;
}
```

##### fork 失败

若操作系统内存耗尽或进程数量过多导致子进程无法创建，`fork()` 会返回 -1。

```c
if (pid < 0)
{
    perror("Failed to fork!");
    return 255;
}
```

##### 调用失败

`execvp()` 在程序调用成功后不会返回，否则会返回 -1。

```c
if (pid == 0)
{
    execvp(args[0], args);
    perror("Failed to execute!");
    return 255;
}
```

### 支持管道



### 支持修改环境变量

使用 `setenv()` 函数即可修改环境变量。

```c
if (strcmp(args[0], "export") == 0)
{
    int n = strlen(args[1]);
    int i = 0;
    while (args[1][i] && args[1][i] != '=')
        i++;
    if (!args[1][i])
        continue;
    args[1][i] = '\0';
    setenv(args[1], args[1] + i + 1, 1);
    continue;
}
```

### 支持文件重定向

### 支持获取环境变量

### 支持临时设置环境变量

### 支持命令别名

### 支持 slash

### 支持括号

### 支持分号

### 支持后台



### 准备测试 shell 程序

将写好的程序拷贝到 `./mini-os/rootfs`，命名为 `init.c`，然后编译为 `init` 。

```shell
[root@zml-HP-Z420-Workstation exp2]# cd mini-os/rootfs
[root@zml-HP-Z420-Workstation rootfs]# vim init.c
[root@zml-HP-Z420-Workstation rootfs]# gcc -static -o init init.c
```

## 运行操作系统

### 打包文件系统

```shell
[root@zml-HP-Z420-Workstation exp2]# cd mini-os/rootfs
[root@zml-HP-Z420-Workstation rootfs]# find . | cpio -oHnewc | gzip > ../initramfs.gz
```

### 运行操作系统

#### 创建启动脚本

创建启动虚拟机的脚本 `run.sh`。

```shell
#!/bin/sh
KERNEL="linux-4.16.2/arch/x86_64/boot/bzImage"
INITRD="mini-os/initramfs.gz"
APPEND="console=ttyS0"

qemu-system-x86_64 \
        -append "$APPEND" \
        -initrd "$INITRD" \
        -kernel "$KERNEL" \
        -nographic \
```

然后给启动脚本加上可执行权限。

```shell
chmod +x run.sh
```

### 运行启动脚本

```shell
[root@zml-HP-Z420-Workstation exp2]# ./run.sh
```
