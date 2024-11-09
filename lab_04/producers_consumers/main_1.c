// Решение Э. Дейкстры задачи «производство-потребление»
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 26 /*размер буфера*/

#define CNT_RUN N

// семафоры
#define BIN_SEM 0
#define BUFF_FULL 1
#define BUFF_EMPTY 2

#define PROD_CNT 3
#define CONS_CNT 4

typedef struct
{
    int write_pos;
    int read_pos;
    char buff[N];
} cbuffer_t;

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


// ПРОИЗВОДИТЕЛЬ
void producer(const int shmid, const int semid)
{
    char alpha = 'a';
    srand(getpid());

    cbuffer_t *cbuf_ptr = shmat(shmid, NULL, 0);
    if (cbuf_ptr == (cbuffer_t *) -1)
    {
        perror("Error: shmat.\n");
        exit(1);
    }

    for (int i = 0; i < CNT_RUN; i++)
    {
        if (semop(semid, start_produce, 2) == -1)
        {
            perror("Error: semop (start_produce).\n");
            exit(1);
        }

        // Производим запись
        *(cbuf_ptr->buff + cbuf_ptr->write_pos) = alpha;
        printf("Producer %d -> %c\n", getpid(), alpha);
        alpha = 'a' + (alpha - 'a' + 1) % 26;
        cbuf_ptr->write_pos = (cbuf_ptr->write_pos + 1) % N;

        sleep(rand() % 2);

        if (semop(semid, stop_produce, 2) == -1)
        {
            perror("Error: semop (stop_produce).\n");
            exit(1);
        }
    }

    if (shmdt(cbuf_ptr) == -1)
    {
        perror("Error: shmdt.\n");
        exit(1);
    }
    exit(0);
}

void consumer(const int shmid, const int semid)
{
    char alpha;
    srand(getpid());

    cbuffer_t *cbuf_ptr = shmat(shmid, NULL, 0);
    if (cbuf_ptr == (cbuffer_t *) -1)
    {
        perror("Error: shmat.\n");
        exit(1);
    }

    for (int i = 0; i < (int)(CNT_RUN * PROD_CNT) / CONS_CNT; i++)
    {
        if (semop(semid, start_consume, 2) == -1)
        {
            perror("Error: semop (start_consume).\n");
            exit(1);
        }

        // Чтение из буфера
        alpha = *(cbuf_ptr->buff + cbuf_ptr->read_pos);
        printf("Consumer %d -> %c\n", getpid(), alpha);
        cbuf_ptr->read_pos = (cbuf_ptr->read_pos + 1) % N;

        sleep(rand() % 3);

        if (semop(semid, stop_consume, 2) == -1)
        {
            perror("Error: semop (stop_consume).\n");
            exit(1);
        }
    }

    if (shmdt(cbuf_ptr) == -1)
    {
        perror("Error: shmdt.\n");
        exit(1);
    }
    exit(0);
}


int main(void)
{
    setbuf(stdout, NULL);
    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t shmkey = ftok("file.txt", 3);
    if (shmkey == (key_t) -1)
    {
        printf("Error: ftok.\n");
        exit(1);
    }
    // права доступа
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    // Функция shmget() создает новый разделяемый сегмент или, если сегмент уже существует, то права доступа подтверждаются
    int shmid = shmget(shmkey, sizeof(cbuffer_t), perms | IPC_CREAT);
    if (shmid == -1)
    {
        printf("Error: shmget.\n");
        exit(1);
    }
    // ПОДКЛЮЧАЕМ СЕГМЕНТ РАЗДЕЛЯЕМОЙ ПАМЯТИ С ID=shmid К АДРЕСНОМУ ПРОСТРАНСТВУ ПРОЦЕССА
    // Вызов shmat() подключает сегмент общей памяти System V с идентификатором shmid к адресному пространству вызывающего процесса
    // Если значение shmaddr равно NULL, то система выбирает подходящий (неиспользуемый) адрес для подключения сегмента
    cbuffer_t *cbuf = shmat(shmid, NULL, 0);
    // ПРОВЕРКА ПОДКЛЮЧЕНИЯ
    // При успешном выполнении shmat() возвращается адрес подключённого общего сегмента памяти; при ошибке возвращается (void *) -1, а в errno содержится код ошибки.
    if (cbuf == (cbuffer_t *) -1)
    {
        printf("Error: shmat.\n");
        exit(1);
    }
    cbuf->read_pos = 0;
    cbuf->write_pos = 0;
    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t semkey = ftok("file.txt", 4);
    if (semkey == (key_t) -1)
    {
        printf("Error: ftok.\n");
        exit(1);
    }
    // ВЫДЕЛЯЕТСЯ ПАМЯТЬ ПОД СЕМАФОРЫ
    // Функция shmget() создает новый разделяемый сегмент или, если сегмент уже существует, то права доступа подтверждаются (возвращается дескриптор)
    int semid = semget(semkey, 3, IPC_CREAT | perms); 
    if (semid == -1)
    {
        printf("Error: semget.\n");
        exit(1);
    }
    // Идентификация семафоров (3 семафора)
    // Функция semctl() позволяет изменять управляющие параметры набора семафоров
    int rc_sb = semctl(semid, BIN_SEM, SETVAL, 1);
    int rc_se = semctl(semid, BUFF_EMPTY, SETVAL, N);
    int rc_sf = semctl(semid, BUFF_FULL, SETVAL, 0);
    // Проверка установки нового значения
    if (rc_se == -1 || rc_sf == -1 || rc_sb == -1)
    {
        perror("Can't semctl.\n");
        exit(1);
    }
    // МАССИВ ID ПРОЦЕССОВ
    pid_t chpid[PROD_CNT + CONS_CNT];
    // ЗАПУСКАЕМ ПРОЦЕССЫ-ПРОИЗВОДИТЕЛИ
    for (int i = 0; i < PROD_CNT; i++)
    {
        chpid[i] = fork();
        if (chpid[i] == -1)
        {
            printf("Error: fork producer.\n");
            exit(1);
        }
        else if (chpid[i] == 0)
        {
            producer(shmid, semid);
        }
    }
    // ЗАПУСКАЕМ ПРОЦЕССЫ-ПОТРЕБИТЕЛИ
    for (int i = PROD_CNT; i < CONS_CNT + PROD_CNT; i++)
    {
        chpid[i] = fork();
        if (chpid[i] == -1)
        {
            printf("Error: fork consumer.\n");
            exit(1);
        }
        else if (chpid[i] == 0)
        {
            consumer(shmid, semid);
        }
    }

    // Ожидание завершение всех children
    for (int i = 0; i < (CONS_CNT + PROD_CNT); i++) 
    {
        int status;
        printf("Wait of final - pid %d\n", chpid[i]);
        if (waitpid(chpid[i], &status, WUNTRACED) == -1)
        {
            printf("Error: waitpid.\n");
            exit(1);
        }

        if (WIFEXITED(status)) 
            printf("Child with pid %d has finished, code: %d\n", chpid[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Child with pid %d has finished by unhandlable signal, signum: %d\n", chpid[i], WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Child with pid %d has finished by signal, signum: %d\n", chpid[i], WSTOPSIG(status));
    }
    // Вызов shmdt() отключает сегмент общей памяти, находящийся по адресу prod_ptr, от адресного пространства вызывающего процесса
    if (shmdt((void *) cbuf) == -1)
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
    // Функция semctl() позволяет изменять управляющие параметры набора семафоров
    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    {
        printf("Error: delete segment.\n");
        exit(1);
    }
    return 0;
}