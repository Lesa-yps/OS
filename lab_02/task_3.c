#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define N 2

int main(void)
{
	pid_t w;
    int wstatus;
	pid_t cpid[N];
	const char *exec_programs[N] = {"./sum.exe", "./two_degree.exe"};
	for (int i = 0; i < N; i++)
	{
		cpid[i] = fork();
		if (cpid[i] == -1)
		{
            perror("fork");
       		exit(EXIT_FAILURE);
      	}
		if (cpid[i] == 0)
		{
			printf("Child: pid = %d, ppid = %d, gr = %d\n", getpid(), getppid(), getpgrp());
			if (execl(exec_programs[i], exec_programs[i], (char *)NULL) == -1) // If execl fails
			{
				perror("execl failed");
				exit(EXIT_FAILURE);
			}	
		}
		else
			printf("Parent: pid = %d, childpid = %d, gr = %d\n", getpid(), cpid[i], getpgrp());
	}
	for (int i = 0; i < N; i++)
	{
        do {
            w = waitpid(cpid[i], &wstatus, WUNTRACED | WCONTINUED);
            if (w == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(wstatus)) {
                printf("exited, status=%d\n", WEXITSTATUS(wstatus));
            } else if (WIFSIGNALED(wstatus)) {
                printf("killed by signal %d\n", WTERMSIG(wstatus));
            } else if (WIFSTOPPED(wstatus)) {
                printf("stopped by signal %d\n", WSTOPSIG(wstatus));
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    }
	return 0;
}