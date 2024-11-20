// Решение Э. Дейкстры задачи «производство-потребление»
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define SIZE_BUF 26 /*размер буфера*/

// семафоры
#define BIN_SEM 0
#define BUFF_FULL 1
#define BUFF_EMPTY 2

#define PROD_CNT 3
#define CONS_CNT 8

// операции над семафорами (уменьшение и увеличение)
#define P -1
#define V  1

/* struct sembuf {
    short sem_num; // индекс семафора
    short sem_op; // операция: увеличение, уменьшение или проверка значения
    short sem_flg; // флаги 
} */  

struct sembuf start_produce[2] = {{BUFF_EMPTY, P, 0}, {BIN_SEM, P, 0}};
struct sembuf stop_produce[2] =  {{BIN_SEM, V, 0}, {BUFF_FULL, V, 0}};
struct sembuf start_consume[2] = {{BUFF_FULL, P, 0}, {BIN_SEM, P, 0}};
struct sembuf stop_consume[2] =  {{BIN_SEM, V, 0}, {BUFF_EMPTY, V, 0}};

// флаг о получении сигнала
int sig_flag = 1;

// Устанавливаем флаг при получении сигнала
void signal_handler(int sig_num)
{
	printf(" pid = %d catch: %d.\n", getpid(), sig_num);
    sig_flag = 0;
}

// ПРОИЗВОДИТЕЛЬ
void producer(char *addr, const int semid)
{
    srand(time(NULL));
    // разбираем буфер на составляющие
    char **ptr_prod = (char **) addr;
    char **ptr_cons = ptr_prod + sizeof(char);
    char *ptr_alpha_now = (char*) (ptr_cons + sizeof(char));
    // цикл длится до получения сигнала, печатая алфавит по кругу
    while (sig_flag)
    {
        // рандомная задержка
        sleep(rand() % 5 + 1);
        // изменение значений 2-ух семафоров (start_produce)
        if (semop(semid, start_produce, 2) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
        // записываем букву в буфер и выводим её
        **ptr_prod = *ptr_alpha_now;
        printf("Producer %d -> %c\n", getpid(), **ptr_prod);
        // производители теперь пишут на следующее место и буква следующая
        if (*ptr_alpha_now == 'z')
        {
            *ptr_prod -= SIZE_BUF;
            *ptr_alpha_now = 'a';
        }
        else
        {
            (*ptr_prod)++;
            (*ptr_alpha_now)++;
        }
        // изменение значений 2-ух семафоров (stop_produce)
        if (semop(semid, stop_produce, 2) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
    }
    exit(0);
}

// ПОТРЕБИТЕЛЬ
void consumer(char *addr, const int semid)
{
    srand(time(NULL));
    // разбираем буфер на составляющие
    char **ptr_prod = (char **) addr;
    char **ptr_cons = ptr_prod + sizeof(char);
    char *ptr_alpha_now = (char*) (ptr_cons + sizeof(char));
    // цикл длится до получения сигнала, читая алфавит по кругу
    while (sig_flag)
    {
        // рандомная задержка
        sleep(rand() % 4 + 1);
        // изменение значений 2-ух семафоров (start_consume)
        if (semop(semid, start_consume, 2) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
        // выводим букву из буфера
        printf("Consumer %d -> %c\n", getpid(), **ptr_cons);
        if ((**ptr_cons) == 'z')
            *ptr_cons -= SIZE_BUF;
        else
            (*ptr_cons)++;
        // изменение значений 2-ух семафоров (stop_consume)
        if (semop(semid, stop_consume, 2) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
    }
    exit(0);
}


int main(void)
{
    setbuf(stdout, NULL);
    // Устанавливаем обработчик сигнала
	// SIGINT - сигнал (Ctrl-C).
	if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
    	perror("Error: signal.\n");
    	exit(EXIT_FAILURE);
    }
    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t shmkey = ftok("file.txt", 3);
    if (shmkey == (key_t) -1)
    {
        perror("Error: ftok.\n");
        exit(1);
    }
    // права доступа
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    // выделяем разделяемую память под буфер + 2 указателя + текущую букву
    // Функция shmget() создает новый разделяемый сегмент или, если сегмент уже существует, то права доступа подтверждаются
    int shmid = shmget(shmkey, (SIZE_BUF + 3) * sizeof(char), perms | IPC_CREAT);
    if (shmid == -1)
    {
        perror("Error: shmget.\n");
        exit(1);
    }
    // Вызов shmat() подключает сегмент общей памяти System V с идентификатором shmid к адресному пространству вызывающего процесса
    // Если значение shmaddr равно NULL, то система выбирает подходящий (неиспользуемый) адрес для подключения сегмента
    char *addr = shmat(shmid, NULL, 0);
    // При успешном выполнении shmat() возвращается адрес подключённого общего сегмента памяти; при ошибке возвращается (void *) -1, а в errno содержится код ошибки.
    if (addr == (char *) -1)
    {
        perror("Error: shmat.\n");
        exit(1);
    }
    // первые 3 позиции буфера разделяемой памяти
    char **ptr_prod = (char **) addr; // текущий адрес производителя
    char **ptr_cons = ptr_prod + sizeof(char); // текущий адрес потребителя
    char *ptr_alpha_now = (char*) (ptr_cons + sizeof(char)); // текущая буква
    // установка стартовых значений
    *ptr_cons = ptr_alpha_now + sizeof(char); // потребитель начинает с первой ячейки буфера
    *ptr_prod = *ptr_cons; // производитель начинает с первой ячейки буфера
    *ptr_alpha_now = 'a'; // первая буква 'a'
    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t semkey = ftok("file.txt", 4);
    if (semkey == (key_t) -1)
    {
        perror("Error: ftok.\n");
        exit(1);
    }
    // выделяем разделяемую память под семафоры
    // Функция shmget() создает новый разделяемый сегмент или, если сегмент уже существует, то права доступа подтверждаются (возвращается дескриптор)
    int semid = semget(semkey, 3, IPC_CREAT | perms); 
    if (semid == -1)
    {
        perror("Error: semget.\n");
        exit(1);
    }
    // Идентификация 3-ёх семафоров
    // Функция semctl() позволяет изменять управляющие параметры набора семафоров
    int rc_bin_sem = semctl(semid, BIN_SEM, SETVAL, 1);
    int rc_buff_empty = semctl(semid, BUFF_EMPTY, SETVAL, SIZE_BUF);
    int rc_buff_full = semctl(semid, BUFF_FULL, SETVAL, 0);
    if (rc_bin_sem == -1 || rc_buff_empty == -1 || rc_buff_full == -1)
    {
        perror("Error: semctl.\n");
        exit(1);
    }
    // МАССИВ ID ПРОЦЕССОВ
    pid_t cpid[PROD_CNT + CONS_CNT];
    // ЗАПУСКАЕМ ПРОЦЕССЫ-ПРОИЗВОДИТЕЛИ
    for (int i = 0; i < PROD_CNT; i++)
    {
        cpid[i] = fork();
        if (cpid[i] == -1)
        {
            perror("Error: fork producer.\n");
            exit(1);
        }
        else if (cpid[i] == 0)
        {
            producer(addr, semid);
        }
    }
    // ЗАПУСКАЕМ ПРОЦЕССЫ-ПОТРЕБИТЕЛИ
    for (int i = PROD_CNT; i < CONS_CNT + PROD_CNT; i++)
    {
        cpid[i] = fork();
        if (cpid[i] == -1)
        {
            perror("Error: fork consumer.\n");
            exit(1);
        }
        else if (cpid[i] == 0)
        {
            consumer(addr, semid);
        }
    }

    // родительский процесс ждет завершения каждого дочернего процесса с помощью wait()
	for (int i = 0; i < (PROD_CNT + CONS_CNT); i++)
	{
		int wait_status;
		pid_t res_waitpid = waitpid(cpid[i], &wait_status, WUNTRACED | WCONTINUED); 		
		if (res_waitpid == -1)
		{
            perror("Error: waitpid.\n");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(wait_status))
            printf("%d exited, status=%d, errno %d\n", cpid[i], WEXITSTATUS(wait_status), errno);
		else if (WIFSIGNALED(wait_status))
            printf("%d killed by signal %d, errno %d\n", cpid[i], WTERMSIG(wait_status), errno);
		else if (WIFSTOPPED(wait_status))
            printf("%d stopped by signal %d, errno %d\n", cpid[i], WSTOPSIG(wait_status), errno);
	}
    // Вызов shmdt() отключает сегмент общей памяти, находящийся по адресу prod_ptr, от адресного пространства вызывающего процесса
    if (shmdt((void *) addr) == -1)
    {
        printf("Error: shmdt.\n");
        exit(1);
    }
    // Функция semctl() позволяет изменять управляющие параметры набора семафоров
    // IPC_RMID используется для пометки сегмента как удаленного.
    if (semctl(semid, 1, IPC_RMID, NULL) == -1)
    {
        printf("Error: delete semafor.\n");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    {
        printf("Error: delete segment.\n");
        exit(1);
    }
    return 0;
}