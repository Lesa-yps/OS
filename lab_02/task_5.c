#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define N 2
#define SIZE_OF_BUF 256

int is_signal = 0;


// Устанавливаем флаг при получении сигнала
void signal_handler(int signal_num)
{
	printf("Catch signal with number = %d.\n", signal_num);
    is_signal = 1;
}


int main(void)
{
	pid_t arr_childpid[N];

	char mess_give[N][SIZE_OF_BUF] = {"Hello from child 1!", "Hi number 2)"};
	char mess_take[N][SIZE_OF_BUF];

	// Устанавливаем обработчик сигнала
	// SIGTSTP - сигнал остановки с терминала (Ctrl-Z).
	if (signal(SIGTSTP, signal_handler) == SIG_ERR)
    {
    	perror("Can't signal.\n");
    	exit(EXIT_FAILURE);
    }

	int file_desc[2];
	// Создаем канал (file_desc[0] - чтение, file_desc[1] - запись)
    if (pipe(file_desc) == -1)
	{
        perror("pipe");
        exit(EXIT_FAILURE);
    }

	for (int i = 0; i < N; i++)
	{
		// Ждём сигнала
		sleep(3);
		arr_childpid[i] = fork();
		if (arr_childpid[i] == -1)
		{
            perror("fork");
       		exit(EXIT_FAILURE);
      	}
		if (arr_childpid[i] == 0)
		{
			/*child*/
			printf("Child: pid = %d, ppid = %d, gr = %d\n", getpid(), getppid(), getpgrp());	
			// Пишем, если есть сигнал
			if (is_signal)
			{
				// Закрываем конец чтения
				close(file_desc[0]);
				write(file_desc[1], mess_give[i], strlen(mess_give[i]) + 1);
				// Закрываем конец записи
				close(file_desc[1]);
				printf("Child №%d sent massage to parent!\n", i + 1);
			}
			else
				printf("Child №%d hadn't signal and didn't sent massage to parent!\n", i + 1);
            exit(EXIT_SUCCESS);
		}
		else
		{
			/*parent*/
			printf("Parent: pid = %d, childpid = %d, gr = %d\n", getpid(), arr_childpid[i], getpgrp());
			
			int wait_status;
			pid_t res_waitpid = wait(&wait_status);
			// Читаем сообщение, если был сигнал
			if (is_signal)
			{
				read(file_desc[0], mess_take[i], sizeof(mess_take[i]));
				printf("Parent received massage: %s\n", mess_take[i]);
			}
			else
				printf("Parent hadn't massage from child №%d\n", i + 1);
    		
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
	}

	// Закрываем конец чтения после завершения работы
    close(file_desc[0]);

	return 0;
}