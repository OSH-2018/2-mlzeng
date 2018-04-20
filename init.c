#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>

void err(char * s)
{
	fprintf(stderr, "%s\n", s);
}

const char * z(const char * s)
{
	return s == NULL ? "" : s;
}

int zsh(char **args)
{
	int i, j, cnt;
	char *pos;

	/* 没有命令 */
	if (!args[0])
		return 0;
	
	/* 处理非括号内分号 */
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

	/* 处理后台运行 */
	i = 0;
	while (args[i]) i++;
	if (i && strcmp(args[i - 1], "&") == 0)
	{
		args[i - 1] = NULL;
		pid_t pid = fork();
		if (pid < 0)
		{
			err("Failed to fork!");
			exit(255);
		}
		if (pid == 0)
		{
			exit(zsh(args));
		}
		return 0;
	}

	/* 处理括号 */
	i = 0;
	if (strcmp(args[0], "(") == 0)
	{
		while (args[i])
			i++;
		if (i && strcmp(args[i - 1], ")") == 0)
		{
			args[0] = NULL;
			args[i - 1] = NULL;
			zsh(args + 1);
			return 0;
		}
		else
		{
			err("Syntax error!");
			return 255;
		}
	}

	/* 处理管道 */
	i = 0;
	while (args[i] && strcmp(args[i] , "|")) i++;
	if (args[i])
	{
		args[i] = NULL;
		int p[2];
		if (pipe(p) < 0)
		{
			err("Failed to create pipe!");
			return 255;
		}
		pid_t pid = fork();
		if (pid < 0)
		{
			err("Failed to fork!");
			exit(255);
		}
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

	/* 处理重定向 */
	/* 输入 */
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
	/* 输出 */
	i = 0;
	while (args[i] && strcmp(args[i], ">")) i++;
	if (args[i])
	{
		int append = 0;
		if (args[i + 1] && strcmp(args[i + 1], ">") == 0) append = 1;
		int stdout_copy_fd = dup(STDOUT_FILENO);
		int fd = open(args[i+1+append], O_WRONLY | O_CREAT | (append ? O_APPEND : 0));
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

	/* 波浪线符号处理 */
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

	/* 获取环境变量 */
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

	/* 临时环境变量 */
	if((pos = strchr(args[0], '=')))
	{
		*pos = '\0';
		char s[strlen(z(getenv(args[0]))) + 10];
		int flag = getenv(args[0]) == NULL;
		strcpy(s, z(getenv(args[0])));
		setenv(args[0], pos + 1, 1);
		int stat = zsh(args + 1);
		if(flag) unsetenv(args[0]); else setenv(args[0], s, 1);
		return stat;
	}

	/* 内建命令 */
	/* 处理别名 */
	{
		char s[strlen(args[0]) + 10];
		s[0] = '\0';
		strcat(s, "__alias__");
		strcat(s, args[0]);
		if (getenv(s))
		{
			args[0] = getenv(s);
			char b[strlen(args[0]) + 10];
			strcpy(b, args[0]);
			unsetenv(s);
			int stat = zsh(args);
			setenv(s, b, 1);
			return stat;
		}
	}
	if (strcmp(args[0], "cd") == 0)
	{
		if (args[1])
			chdir(args[1]);
		return 0;
	}
	if (strcmp(args[0], "pwd") == 0)
	{
		char wd[4096];
		puts(getcwd(wd, 4096));
		return 0;
	}
	if (strcmp(args[0], "export") == 0)
	{		
		int i = 0;
		while (args[1][i] && args[1][i] != '=') i++;
		if (!args[1][i]) return 0;
		args[1][i] = '\0';
		setenv(args[1], args[1] + i + 1, 1);
		return 0;
	}
	if (strcmp(args[0], "alias") == 0)
	{
		int i = 0;
		while (args[1][i] && args[1][i] != '=') i++;
		if (!args[1][i]) return 0;
		args[1][i] = '\0';
		char s[strlen(args[1]) + 10];
		s[0] = '\0';
		strcat(s, "__alias__");
		strcat(s, args[1]);
		setenv(s, args[1] + i + 1, 1);
		return 0;
	}
	if (strcmp(args[0], "unalias") == 0)
	{
		char s[strlen(args[1]) + 10];
		s[0] = '\0';
		strcat(s, "__alias__");
		strcat(s, args[1]);
		unsetenv(s);
		return 0;
	}
	if (strcmp(args[0], "exit") == 0)
		exit(0);

	/* 外部命令 */
	pid_t pid = fork();
	if (pid < 0)
	{
		err("Failed to fork!");
		exit(255);
	}
	if (pid == 0)
	{
		/* 子进程 */
		execvp(args[0], args);
		err("Failed to execute!");
		/* execvp失败 */
		exit(255);
	}

	/* 等待子进程结束 */
	int stat;
	waitpid(pid, &stat, WUNTRACED);
	return stat;
}

int main()
{
	/* 输入的命令行 */
	char inp[256];
	char cmd[1024];
	/* 命令行拆解成的各部分，以空指针结尾 */
	char *args[1024];
	while (1)
	{
		/* 提示符 */
		printf("# ");
		fflush(stdin);
		fgets(inp, 256, stdin);
		if (ferror(stdin))
		{
			err("Input stream error!");
			continue;
		}
		if (feof(stdin))
		{
			break;
		}

		/* 处理原输入 */
		int i;
		int j = 0;
		for (i = 0; inp[i] != '\n'; i++)
		{
			if (strchr("();&|<>", inp[i])) cmd[j++] = ' ';
			cmd[j++] = inp[i];
			if (strchr("();&|<>", inp[i])) cmd[j++] = ' ';
		}
		cmd[j] = '\0';

		/* 按空白字符拆解命令行 */
		args[0] = cmd;
		while (isblank(args[0][0])) args[0]++;
		for (i = 0; *args[i]; i++)
		{
			for (args[i + 1] = args[i] + 1; *args[i + 1]; args[i + 1]++)
			{
				if (isblank(*args[i + 1]))
				{
					*args[i + 1] = '\0';
					args[i + 1]++;
					while (*args[i + 1] && isblank(*args[i + 1]))
						args[i + 1]++;
					break;
				}
			}
		}
		args[i] = NULL;

		/* 递归处理 */
		zsh(args);
	}
}