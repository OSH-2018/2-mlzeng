#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>

int main()
{
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];
    while (1)
    {
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        if (ferror(stdin))
        {
            perror("Input stream error!");
            continue;
        }
        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        /* 拆解命令行 */
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i + 1] = args[i] + 1; *args[i + 1]; args[i + 1]++)
                if (isblank(*args[i + 1]))
                {
                    *args[i + 1] = '\0';
                    args[i + 1]++;
                    while (*args[i + 1] && isblank(*args[i + 1]))
                        args[i + 1]++;
                    break;
                }
        args[i] = NULL;

        /* 没有输入命令 */
        if (!args[0])
            continue;

        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0)
        {
            if (args[1])
                chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "pwd") == 0)
        {
            char wd[4096];
            puts(getcwd(wd, 4096));
            continue;
        }
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
        if (strcmp(args[0], "exit") == 0)
            return 0;

        /* 外部命令 */
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Failed to fork!");
            return 255;
        }
        if (pid == 0)
        {
            /* 子进程 */
            execvp(args[0], args);
            perror("Failed to execute!");
            /* execvp失败 */
            return 255;
        }
        /* 父进程 */
        wait(NULL);
    }
}