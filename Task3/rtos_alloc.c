#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int foo = 0;

int main()
{
    int bar[2], baz;
    pipe(bar);

    pid_t pid = fork();

    if (pid > 0)
    {
        wait(&baz);
        read(bar[0], &bar[1], sizeof(int));
        printf("%d-%d-%d-%d\n", foo, WEXITSTATUS(baz), bar[0], bar[1]);
    }
    else
    {
        foo = pid;
        write(bar[1], &pid, sizeof(pid));
        printf("%d-%d\n", pid, bar[0]);
        return bar [0];
    }

    return 1;
}