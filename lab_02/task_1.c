#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#define N 2

int main(void)
{
	pid_t arr_childpid[N];
	for (int i = 0; i < N; i++)
	{
		arr_childpid[i] = fork();
		if (arr_childpid[i] == 0)
		{
			printf("Child before sleep: pid = %d, ppid = %d, gr = %d\n", getpid(), getppid(), getpgrp());
			sleep(2);
			printf("Child after sleep: pid = %d, ppid = %d, gr = %d\n", getpid(), getppid(), getpgrp());
			exit(0);		
		}
		else
		{
			printf("Parent: pid = %d, childpid = %d, gr = %d\n", getpid(), arr_childpid[i], getpgrp());
		}
	}
	return 0;
}
