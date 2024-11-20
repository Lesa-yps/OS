#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define N 2
#define SIZE_OF_BUF 256

int main(void)
{
	pid_t cpid[N];

	const char *mess_give[N] = {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "BBB"};
	char *mess_take = malloc(SIZE_OF_BUF);

	int fd[N];
	// Создается канал (fd[0] - чтение, fd[1] - запись)
    if (pipe(fd) == -1)
	{
        perror("pipe");
        exit(EXIT_FAILURE);
    }
	// цикл создает дочерние процессы с помощью fork()
	for (int i = 0; i < N; i++)
	{
		cpid[i] = fork();
		if (cpid[i] == -1)
		{
            perror("Error fork");
       		exit(EXIT_FAILURE);
      	}
		if (cpid[i] == 0)
		{
			/*child*/
			// Закрывается конец чтения
			close(fd[0]);
			// записывается сообщение
            write(fd[1], (mess_give)[i], strlen((mess_give)[i]));
			printf("Child pid = %d sent massage %s to parent ppid = %d\n", getpid(), (mess_give)[i], getppid());
            exit(EXIT_SUCCESS); 
		}
	}
	// родительский процесс ждет завершения каждого дочернего процесса с помощью wait()
	for (int i = 0; i < N; i++)
	{
		/*parent*/
		int wait_status;
		pid_t res_waitpid = waitpid(cpid[i], &wait_status, WUNTRACED | WCONTINUED); 		
		if (res_waitpid == -1)
		{
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
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
		else if (WIFCONTINUED(wait_status))
		{
            printf("continued\n");
        }
	}

	// Закрывается конец записи
    close(fd[1]);
	for (int i = 0; i < N; i++)
	{
		memset(mess_take, 0, SIZE_OF_BUF); // Обнуляется буфер перед чтением
		// Читается сообщение
        read(fd[0], mess_take, strlen((mess_give)[i]));
        printf("Parent received massage: %s\n", mess_take);
	}
	// контрольное третье чтение
	mess_take[0] = '\0';
	read(fd[0], mess_take, sizeof(mess_take));
	printf("Parent received massage: %s\n", mess_take);

	return 0;
}