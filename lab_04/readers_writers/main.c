// Монитор Хоара «Читатели-писатели»
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// семафоры
#define FLAG_EDIT 0
#define ACTIVE_WRITER 1
#define ACTIVE_READERS 2
#define WRITE_QUEUE 3
#define READ_QUEUE 4

#define WRIT_CNT 3
#define READ_CNT 5

// операции над семафорами (уменьшение и увеличение)
#define P -1
#define V  1

/* struct sembuf {
    short sem_num; // индекс семафора
    short sem_op; // операция: увеличение, уменьшение или проверка значения
    short sem_flg; // флаги 
} */  

// читатель может начать читать, если нет пишущего писателя или нет ждущих писателей
struct sembuf start_read[] = {{READ_QUEUE, V, 0},
                            {ACTIVE_WRITER, 0, 0},
                            {WRITE_QUEUE, 0, 0},
                            {ACTIVE_READERS, V, 0},
                            {READ_QUEUE, P, 0}};

struct sembuf stop_read[] = {{ACTIVE_READERS, P, 0}};

// писатель может писать, если нет активных читателей и нет активных писателей
struct sembuf start_write[] = {{WRITE_QUEUE, V, 0},
                                {ACTIVE_READERS, 0, 0},
                                {FLAG_EDIT, P, 0},
                                {ACTIVE_WRITER, 0, 0},
                                {ACTIVE_WRITER, V, 0},
                                {WRITE_QUEUE, P, 0}};

struct sembuf stop_write[] = {{ACTIVE_WRITER, P, 0},
                            {FLAG_EDIT, V, 0}};


// флаг о получении сигнала
int sig_flag = 1;

// Устанавливаем флаг при получении сигнала
void signal_handler(int sig_num)
{
	printf(" pid = %d catch: %d.\n", getpid(), sig_num);
    sig_flag = 0;
}

// ПИСАТЕЛЬ
void writer(char *addr, const int semid)
{
    srand(time(NULL));
    while (sig_flag)
    {
        sleep(rand() % 4 + 1);
        // изменение значений 6-и семафоров (start_write)
        if (semop(semid, start_write, 6) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop (start write) pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
        // увеличиваем букву и выводим
        if (*addr == 'z')
            *addr = 'a';
        else
            (*addr)++;
        printf("Writer %d -> %c\n", getpid(), *addr);
        // изменение значений 2-ух семафоров (stop_write)
        if (semop(semid, stop_write, 2) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop (stop write) pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
    }
    exit(0);
}

// ЧИТАТЕЛЬ
void reader(char *addr, const int semid)
{
    srand(time(NULL));
    while (sig_flag)
    {
        sleep(rand() % 3 + 1);
        if (semop(semid, start_read, 5) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop (start read) pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
        printf("Reader %d -> %c\n", getpid(), *addr);
        if (semop(semid, stop_read, 1) == -1)
        {
            char err_msg[100];
            sprintf(err_msg, "Error: semop (stop read) pid = %d, errno %d", getpid(), errno);
            perror(err_msg);
            exit(1);
        }
    }
    exit(0);
}

// Функция semctl() позволяет изменять управляющие параметры набора семафоров (установка значения семафоров)
int init_sems(const int semid)
{
    int rc = 0;
    if (semctl(semid, ACTIVE_WRITER, SETVAL, 0) == -1)
        rc = -1;
    if (semctl(semid, ACTIVE_READERS, SETVAL, 0) == -1)
        rc = -1;
    if (semctl(semid, WRITE_QUEUE, SETVAL, 0) == -1)
        rc = -1;
    if (semctl(semid, READ_QUEUE, SETVAL, 0) == -1)
        rc = -1;
    if (semctl(semid, FLAG_EDIT, SETVAL, 1) == -1)
        rc = -1;
    return rc;
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
    key_t shmkey = ftok("file.txt", 1);
    if (shmkey == (key_t) -1)
    {
        perror("Error: ftok.\n");
        exit(1);
    }
    // права доступа
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    // Функция shmget() создает новый разделяемый сегмент или, если сегмент уже существует, то права доступа подтверждаются
    int shmid = shmget(shmkey, sizeof(int), perms | IPC_CREAT);
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
    *addr = 'a';
    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t semkey = ftok("file.txt", 2);
    if (semkey == (key_t) -1)
    {
        perror("Error: ftok.\n");
        exit(1);
    }
    // Функция shmget() создает новый разделяемый сегмент или, если сегмент уже существует, то права доступа подтверждаются (возвращается дескриптор)
    int semid = semget(semkey, 5, IPC_CREAT | perms); 
    if (semid == -1)
    {
        perror("Error: semget.\n");
        exit(1);
    }
    // Функция semctl() позволяет изменять управляющие параметры набора семафоров (установка значения семафоров)
    if (init_sems(semid) == -1)
    {
        perror("Error: semctl.\n");
        exit(1);
    }
    // МАССИВ ID ПРОЦЕССОВ
    pid_t cpid[WRIT_CNT + READ_CNT];
    // ЗАПУСКАЕМ ПРОЦЕССЫ-ПИСАТЕЛИ
    for (int i = 0; i < WRIT_CNT; i++)
    {
        cpid[i] = fork();
        if (cpid[i] == -1)
        {
            perror("Error: fork writer.\n");
            exit(1);
        }
        else if (cpid[i] == 0)
        {
            writer(addr, semid);
        }
    }
    // ЗАПУСКАЕМ ПРОЦЕССЫ-ЧИТАТЕЛИ
    for (int i = WRIT_CNT; i < WRIT_CNT + READ_CNT; i++)
    {
        cpid[i] = fork();
        if (cpid[i] == -1)
        {
            perror("Error: fork reader.\n");
            exit(1);
        }
        else if (cpid[i] == 0)
        {
            reader(addr, semid);
        }
    }

    // родительский процесс ждет завершения каждого дочернего процесса с помощью wait()
	for (int i = 0; i < (WRIT_CNT + READ_CNT); i++)
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
    // Вызов shmdt() отключает сегмент общей памяти, находящийся по адресу addr, от адресного пространства вызывающего процесса
    if (shmdt((void *) addr) == -1)
    {
        perror("Error: shmdt.\n");
        exit(1);
    }
    // Функция semctl() позволяет изменять управляющие параметры набора семафоров
    // IPC_RMID используется для пометки сегмента как удаленного.
    if (semctl(semid, 1, IPC_RMID, NULL) == -1)
    {
        perror("Error: delete semafor.\n");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    {
        perror("Error: delete segment.\n");
        exit(1);
    }
    return 0;
}