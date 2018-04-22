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

### 程序整体架构

采用递归式实现，将复杂的 shell 命令不断分解成更简单的 shell 命令，然后再运行对应的二进制文件。往后可以看到，递归式实现非常利于内存管理和环境变量、文件描述符等处理，并且逻辑非常清晰，不同功能部分的代码耦合度低（增删功能只需要在对应位置增删代码）。

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

##### 管道创建失败

`pipe()` 创建管道失败后会返回 -1。

```c
if (pipe(p) < 0)
{
	err("Failed to create pipe!");
	return 255;
}
```

### 支持括号

只处理出现在最外层且在最前面的括号，不同语句中的括号将被后面的分号处理部分分开处理，同一个语句中嵌套的括号将在递归过程中被这部分处理。

```c
i = 0;
if (strcmp(args[0], "(") == 0)
{
	while (args[i]) i++;
	args[0] = NULL;
	args[i - 1] = NULL;
	zsh(args + 1);
	return 0;
}
```

### 支持分号

用一个计数器 cnt 判断分号是否在括号中，直到找到第一个不在括号中的分号，处理前面的部分，然后递归处理后面的部分。

```c
i = 0;
cnt = 0;
while (args[i])
{
	if (strcmp(args[i], "(") == 0) cnt++;
	if (strcmp(args[i], ")") == 0) cnt--;
	if (strcmp(args[i], ";") == 0 && !cnt) break;
	i++;
}
if (args[i])
{
	args[i] = NULL;
	zsh(args);
	zsh(args + i + 1);
	return 0;
}
```

### 支持后台运行

`fork` 出子进程递归处理 `&` 符号前的整块语句，父进程不使用 `waitpid` 即可。

```c
i = 0;
while (args[i]) i++;
if (i && strcmp(args[i - 1], "&") == 0)
{
	args[i - 1] = NULL;
	pid_t pid = fork();
	if (pid == 0) exit(zsh(args));
	return 0;
}
```

测试样例：

```shell
# echo start;((sleep 2;echo 2s)&;(sleep 1;echo 1s);(sleep 2;echo 3s))&;echo 0s
start
0s
# 1s
2s
3s
```

1s 之前出现 # 号是因为输出 0s 之后当前的 shell 运行结束，下一个 shell 已经开始运行了。这是符合期望的。

### 支持管道

从左向右扫描到第一个 `|` 之后，创建管道，覆盖对应的文件描述符，处理前面的部分的同时 `fork` 出一个进程递归处理后面的部分，父进程需要在运行结束前关闭管道并**恢复**原来的文件描述符表（子进程可以不用因为直接被回收掉了）。

```c
i = 0;
while (args[i] && strcmp(args[i] , "|")) i++;
if (args[i])
{
	args[i] = NULL;
	int p[2];
	pipe(p)；
	pid_t pid = fork();
	if (pid == 0)
	{
		close(p[0]);
		dup2(p[1], STDOUT_FILENO);
		int stat = zsh(args);
		close(p[1]);
		exit(stat);
	}
	close(p[1]);
	int stdin_copy_fd = dup(STDIN_FILENO);
	dup2(p[0], STDIN_FILENO);
	zsh(args + i + 1);
	close(p[0]);
	dup2(stdin_copy_fd, STDIN_FILENO);
	int stat;
	waitpid(pid, &stat, WUNTRACED);
	return stat;
}
```

### 支持文件重定向

发现重定向符号后，覆盖对应的文件描述符，然后将重定向符号及其对应文件名去掉，直到剩下一个没有重定向符号的 shell 命令，递归处理之，结束后恢复原来的文件描述符。

```c
i = 0;
while (args[i] && strcmp(args[i], "<")) i++;
if (args[i])
{
	int stdin_copy_fd = dup(STDIN_FILENO);
	int fd = open(args[i+1], O_RDONLY);
	dup2(fd, STDIN_FILENO);
	while (args[i + 2])
	{
		args[i] = args[i + 2];
		i++;
	}
	args[i] = NULL;
	int stat = zsh(args);
	close(fd);
	dup2(stdin_copy_fd, STDIN_FILENO);
	return stat;
}
i = 0;
while (args[i] && strcmp(args[i], ">")) i++;
if (args[i])
{
	int append = 0;
	if (args[i + 1] && strcmp(args[i + 1], ">") == 0) append = 1;
	int stdout_copy_fd = dup(STDOUT_FILENO);
	int fd = open(args[i + 1 + append], O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC));
	dup2(fd, STDOUT_FILENO);
	while (args[i + 2 + append])
	{
		args[i] = args[i + 2 + append];
		i++;
	}
	args[i] = NULL;
	int stat = zsh(args);
	close(fd);
	dup2(stdout_copy_fd, STDOUT_FILENO);
	return stat;
}
```

测试样例：

```shell
# cat > a
echo Hello
# ./init < a > b
# cat b
Hello
```

```shell
# cat > a
Hello
# cat >> a
Hello
# cat < a | cat | cat | tee b | cat | cat | cat > c
# cat a
Hello
Hello
# cat b
Hello
Hello
# cat c
Hello
Hello
#
```

### 支持 slash

查找 `~` 替换为 `$HOME` ，交给后面部分处理。

```c
i = 0;
while (args[i])
{
	char* pos;
	if (pos = strchr(args[i], '~'))
	{
		*pos = '\0';
		char s[strlen(args[i]) + 10];
		s[0]='\0';
		strcat(s, args[i]);
		strcat(s, "$HOME");
		strcat(s, pos + 1);
		args[i] = s;
		return(zsh(args));
	}
	i++;
}
```

### 支持获取环境变量

调用 `getenv()` 函数即可修改环境变量，做简单的字符串替换后递归处理即可。

```c
i = 0;
while (args[i])
{
	if (pos = strchr(args[i], '$'))
	{
		j = 0;
		while (isalnum(pos[j + 1]) || pos[j + 1] == '_')
		{
			pos[j] = pos[j + 1];
			j++;
		}
		pos[j] = '\0';
		char s[strlen(z(getenv(pos))) + strlen(pos + j + 1) + 10];
		s[0]='\0';
		strcat(s, z(getenv(pos)));
		strcat(s, pos + j + 1);
		args[i] = s;
		return(zsh(args));
	}
	i++;
}
```

### 支持临时设置环境变量

设置环境变量，递归处理后面的 shell 命令，结束后还原环境变量即可。

```c
if ((pos = strchr(args[0], '=')))
{
	*pos = '\0';
	char s[strlen(z(getenv(args[0]))) + 10];
	int flag = getenv(args[0]) == NULL;
	strcpy(s, z(getenv(args[0])));
	setenv(args[0], pos + 1, 1);
	int stat = zsh(args + 1);
	if (flag) unsetenv(args[0]); else setenv(args[0], s, 1);
	return stat;
}
```

### 支持修改环境变量

这应该作为 shell 内置命令来实现，调用 `setenv()` 函数即可修改环境变量。

```c
if (strcmp(args[0], "export") == 0)
{
    int n = strlen(args[1]);
    int i = 0;
    while (args[1][i] && args[1][i] != '=') i++;
    if (!args[1][i]) continue;
    args[1][i] = '\0';
    setenv(args[1], args[1] + i + 1, 1);
    continue;
}
```

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
