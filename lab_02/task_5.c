#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define N 2
#define SIZE_OF_BUF 256

int sig_flag = 0;


// Устанавливаем флаг при получении сигнала
void signal_handler(int signal_num)
{
	printf(" Catch: %d.\n", signal_num);
    sig_flag = 1;
}


int main(void)
{
	pid_t arr_childpid[N];
	int fd[2];

	char mess_give[N][SIZE_OF_BUF] = {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "BBB"};
	char mess_take[N + 1][SIZE_OF_BUF];

	// Устанавливаем обработчик сигнала
	// SIGINT - сигнал (Ctrl-C).
	if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
    	perror("Can't signal.\n");
    	exit(EXIT_FAILURE);
    }
	sleep(3);

	// Создаем канал (fd[0] - чтение, fd[1] - запись)
    if (pipe(fd) == -1)
	{
        perror("Error with pipe");
        exit(EXIT_FAILURE);
    }

	

	for (int i = 0; i < N; i++)
	{
		
		arr_childpid[i] = fork();
		if (arr_childpid[i] == -1)
		{
            perror("Error with fork");
       		exit(EXIT_FAILURE);
      	}
		if (arr_childpid[i] == 0)
		{
			/*child*/
			printf("Child: pid = %d, ppid = %d, gr = %d\n", getpid(), getppid(), getpgrp());	
			// Пишем, если есть сигнал
			if (sig_flag)
			{
				printf("Child pid = %d; receive signal!\n", getpid());
				// Закрываем конец чтения
				close(fd[0]);
				write(fd[1], mess_give[i], strlen(mess_give[i]) + 1);				
			}
			else
				printf("Child pid = %d; no signal!\n", getpid());
            exit(EXIT_SUCCESS);
		}
	}

	for (int i = 0; i < N; i++)
	{
		int wait_status;
        pid_t child_pid = waitpid(arr_childpid[i], &wait_status, WUNTRACED);
		
		if (child_pid == -1)
        {
            perror("Error with waitpid");
            exit(EXIT_FAILURE);
        }
        printf("Child: pid = %d ", child_pid);
        if (WIFEXITED(wait_status))
        {
            printf("exited, status=%d\n", WEXITSTATUS(wait_status));
        }
        else if (WIFSIGNALED(wait_status))
        {
            printf("killed by signal %d\n", WTERMSIG(wait_status));
        }
        else if (WIFSTOPPED(wait_status))
        {
            printf("stopped by signal %d\n", WSTOPSIG(wait_status));
        }
        close(fd[1]);
		int j = 0;
		char a;
        do 
		{
			read(fd[0], &a, sizeof(a));
			mess_take[i][j++] = a;
		} while (a != '\0');
		printf("Parent received massage: %s\n", mess_take[i]);
	}
	// mess_take[N][1] = '\0';
    for (int i = 0; i < sizeof(mess_take[N]); i++)
        mess_take[N][i] = 0;
    read(fd[0], mess_take[N], sizeof(mess_take[N]));
    printf("Parent received massage: %s\n", mess_take[N]); // 3-е контрольное чтение

	return 0;
}