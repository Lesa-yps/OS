// Решение Э. Дейкстры задачи «производство-потребление»
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 26 /*размер буфера*/

// семафоры
#define BIN_SEM 0
#define BUFF_FULL 1
#define BUFF_EMPTY 2

#define PROD_CNT 3
#define CONS_CNT 4

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
    // ИНИЦИАЛИЗИРУЕМ РАНДОМ
    srand(getpid());
    // ПОДКЛЮЧАЕМ СЕГМЕНТ РАЗДЕЛЯЕМОЙ ПАМЯТИ С ID=shmid К АДРЕСНОМУ ПРОСТРАНСТВУ ПРОЦЕССА
    // Вызов shmat() подключает сегмент общей памяти System V с идентификатором shmid к адресному пространству вызывающего процесса
    // Если значение shmaddr равно NULL, то система выбирает подходящий (неиспользуемый) адрес для подключения сегмента
    char *addr = shmat(shmid, NULL, 0);
    // ПРОВЕРКА ПОДКЛЮЧЕНИЯ
    // При успешном выполнении shmat() возвращается адрес подключённого общего сегмента памяти; при ошибке возвращается (void *) -1, а в errno содержится код ошибки.
    if (addr == (char *) -1)
    {
        printf("Error: shmat.\n");
        exit(1);
    }
    // РАЗБИРАЕМ УКАЗАТЕЛИ ИЗ НАЧАЛА БУФЕРА
    char **prod_ptr = (char **) addr;
    char **cons_ptr = prod_ptr + sizeof(char);
    char *alpha_ptr = (char*) (cons_ptr + sizeof(char));
    // ЗАПИСЬ ВСЕХ БУКВ АЛФАВИТА
    while ((*alpha_ptr) <= 'z')
    {
        // ИЗМЕНЕНИЕ ЗНАЧЕНИЙ 2-УХ СЕМАФОРОВ (start_produce)
        // значение семафора можно изменять с помощью системного вызова semop()
        int sem_op_p = semop(semid, start_produce, 2);
        if ((*alpha_ptr) <= 'z')
        {
            // ПРОВЕРКА УСПЕШНОСТИ ОПЕРАЦИИ
            if (sem_op_p == -1)
            {
                printf("Error: semop.\n");
                exit(1);
            }
            // ЗАПИСЫВАЕМ БУКВУ В БУФЕР И ВЫВОДИМ ЕЁ
            **prod_ptr = *alpha_ptr;
            printf("Producer %d -> %c\n", getpid(), **prod_ptr);
            // ПРОИЗВОДИТЕЛИ ТЕПЕРЬ ПИШУТ НА 1 ДАЛЬШЕ, А БУКВА НА 1 БОЛЬШЕ
            (*prod_ptr)++;
            (*alpha_ptr)++;
            // РАНДОМНАЯ ЗАДЕРЖКА
            sleep(rand() % 2);
        }
        // ИЗМЕНЕНИЕ ЗНАЧЕНИЙ 2-УХ СЕМАФОРОВ (stop_produce)
        // значение семафора можно изменять с помощью системного вызова semop()
        int sem_op_v = semop(semid, stop_produce, 2);
        // ПРОВЕРКА УСПЕШНОСТИ ОПЕРАЦИИ
        if (sem_op_v == -1)
        {
            printf("Error: semop.\n");
            exit(1);
        }
    }
    // Вызов shmdt() отключает сегмент общей памяти, находящийся по адресу addr, от адресного пространства вызывающего процесса
    // ОТКЛЮЧАЕМ СЕГМЕНТ РАЗДЕЛЯЕМОЙ ПАМЯТИ ОТ АДРЕСНОГО ПРОСТРАНСТВА ПРОЦЕССА
    if (shmdt((void *) addr) == -1)
    {
        perror("Error: shmdt.\n");
        exit(1);
    }
    // УСПЕХ
    exit(0);
}

// ПОТРЕБИТЕЛЬ
void consumer(const int shmid, const int semid)
{
    setbuf(stdout, NULL);
    // ИНИЦИАЛИЗИРУЕМ РАНДОМ
    srand(getpid());
    // ПОДКЛЮЧАЕМ СЕГМЕНТ РАЗДЕЛЯЕМОЙ ПАМЯТИ С ID=shmid К АДРЕСНОМУ ПРОСТРАНСТВУ ПРОЦЕССА
    // Вызов shmat() подключает сегмент общей памяти System V с идентификатором shmid к адресному пространству вызывающего процесса
    // Если значение shmaddr равно NULL, то система выбирает подходящий (неиспользуемый) адрес для подключения сегмента
    char *addr = shmat(shmid, NULL, 0);
    // ПРОВЕРКА ПОДКЛЮЧЕНИЯ
    // При успешном выполнении shmat() возвращается адрес подключённого общего сегмента памяти; при ошибке возвращается (void *) -1, а в errno содержится код ошибки.
    if (addr == (char *) -1)
    {
        printf("Error: shmat.\n");
        exit(1);
    }
    // РАЗБИРАЕМ УКАЗАТЕЛИ ИЗ НАЧАЛА БУФЕРА
    char **prod_ptr = (char **) addr;
    char **cons_ptr = prod_ptr + sizeof(char);
    char *alpha_ptr = (char*) (cons_ptr + sizeof(char));
    // ЧТЕНИЕ ВСЕХ БУКВ АЛФАВИТА
    while ((*alpha_ptr) != '~')
    {
        // ИЗМЕНЕНИЕ ЗНАЧЕНИЙ 2-УХ СЕМАФОРОВ (start_consume)
        // значение семафора можно изменять с помощью системного вызова semop()
        int sem_op_p = semop(semid, start_consume, 2);
        if ((*alpha_ptr) != '~')
        {
            // ПРОВЕРКА УСПЕШНОСТИ ОПЕРАЦИИ
            if (sem_op_p == -1)
            {
                printf("Error: semop.\n");
                exit(1);
            }
            // ВЫВОДИМ БУКВУ ИЗ БУФЕРА И СДВИГАЕМ УКАЗАТЕЛЬ ПОТРЕБИТЕЛЯ
            printf("Consumer %d -> %c\n", getpid(), **cons_ptr);
            if ((**cons_ptr) == 'z')
            {
                *alpha_ptr = '~'; // чтоб сообщить всем остальным потребителям о завершении потребления
                struct sembuf help_consume[1] = {{BUFF_FULL, V, 0}}; // чтоб помочь разблокировать следующих потребителей
                if (semop(semid, help_consume, 1) == -1)
                {
                    printf("Error: semop.\n");
                    exit(1);
                }
            }
            else
            {
                (*cons_ptr)++;
                // РАНДОМНАЯ ЗАДЕРЖКА
                sleep(rand() % 3);
            }
        }
        // ИЗМЕНЕНИЕ ЗНАЧЕНИЙ 2-УХ СЕМАФОРОВ (stop_consume)
        // значение семафора можно изменять с помощью системного вызова semop()
        int sem_op_v = semop(semid, stop_consume, 2);
        // ПРОВЕРКА УСПЕШНОСТИ ОПЕРАЦИИ
        if (sem_op_v == -1)
        {
            printf("Error: semop.\n");
            exit(1);
        }
    }
    // Вызов shmdt() отключает сегмент общей памяти, находящийся по адресу addr, от адресного пространства вызывающего процесса
    // ОТКЛЮЧАЕМ СЕГМЕНТ РАЗДЕЛЯЕМОЙ ПАМЯТИ ОТ АДРЕСНОГО ПРОСТРАНСТВА ПРОЦЕССА
    if (shmdt((void *) addr) == -1)
    {
        printf("Error: shmdt.\n");
        exit(1);
    }
    // УСПЕХ
    exit(0);
}


int main(void)
{
    setbuf(stdout, NULL);
    char** prod_ptr;
    char** cons_ptr;
    char* alpha_ptr;
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
    int shmid = shmget(shmkey, (N + 3) * sizeof(char), perms | IPC_CREAT);
    if (shmid == -1)
    {
        printf("Error: shmget.\n");
        exit(1);
    }
    // ПОДКЛЮЧАЕМ СЕГМЕНТ РАЗДЕЛЯЕМОЙ ПАМЯТИ С ID=shmid К АДРЕСНОМУ ПРОСТРАНСТВУ ПРОЦЕССА
    // Вызов shmat() подключает сегмент общей памяти System V с идентификатором shmid к адресному пространству вызывающего процесса
    // Если значение shmaddr равно NULL, то система выбирает подходящий (неиспользуемый) адрес для подключения сегмента
    char *addr = shmat(shmid, NULL, 0);
    // ПРОВЕРКА ПОДКЛЮЧЕНИЯ
    // При успешном выполнении shmat() возвращается адрес подключённого общего сегмента памяти; при ошибке возвращается (void *) -1, а в errno содержится код ошибки.
    if (addr == (char *) -1)
    {
        printf("Error: shmat.\n");
        exit(1);
    }
    // УКАЗАТЕЛЬ НА БУФЕР В РАЗДЕЛЯЕМОЙ ПАМЯТИ
    prod_ptr = (char **) addr; // ТЕКУЩИЙ АДРЕС ПРОИЗВОДИТЕЛЯ
    cons_ptr = prod_ptr + sizeof(char); // ТЕКУЩИЙ АДРЕС ПОТРЕБИТЕЛЯ
    alpha_ptr = (char*) (cons_ptr + sizeof(char)); // ТЕКУЩАЯ БУКВА
    // УСТАНОВКА СТАРТОВЫХ ЗНАЧЕНИЙ
    *cons_ptr = alpha_ptr + sizeof(char); // ПОТРЕБИТЕЛЬ НАЧИНАЕТ С ПЕРВОЙ ЯЧЕЙКИ БУФЕРА
    *prod_ptr = *cons_ptr; // ПРОИЗВОДИТЕЛЬ НАЧИНАЕТ С ПЕРВОЙ ЯЧЕЙКИ БУФЕРА
    *alpha_ptr = 'a'; // НАЧИНАЕМ С БУКВЫ А
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
    if (shmdt((void *) prod_ptr) == -1)
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