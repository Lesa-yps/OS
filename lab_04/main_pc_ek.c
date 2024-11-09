#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define N_CONS 4
#define N_PROD 3

#define P -1
#define V 1

#define SEM_EMPTY 0
#define SEM_FULL 1
#define SEM_BINARY 2

#define SIZE 26

int flag = 1;

void sig_handler(int sig_num)
{
    flag = 0;
    printf("pid = %d, Catch signal %d\n", getpid(), sig_num);
}

// sembuf -> {sem_num, sem_op, sem_flg}
struct sembuf start_produce[2] = {{SEM_EMPTY, P, 0}, {SEM_BINARY, P, 0}};
struct sembuf stop_produce[2] = {{SEM_BINARY, V, 0}, {SEM_FULL, V, 0}};
struct sembuf start_consume[2] = {{SEM_FULL, P, 0}, {SEM_BINARY, P, 0}};
struct sembuf stop_consume[2] = {{SEM_BINARY, V, 0}, {SEM_EMPTY, V, 0}};

void producer(const int semid, const int shmid)
{
    char *addr = (char *)shmat(shmid, 0, 0);
    if (addr == (char *)-1)
    {
        perror("shmat\n");
        exit(1);
    }
    char **prod_pos = (char **)addr;
    char **cons_pos = prod_pos + 1;
    char *cur_letter = (char *)(cons_pos + 1);
    srand(time(NULL));
    while (flag)
    {
        sleep(rand() % 2 + 1);
        if (semop(semid, start_produce, 2) == -1)
        {
            perror("semop start produce\n");
            exit(1);
        }
        **prod_pos = *cur_letter;
        printf("Producer %d -> %c\n", getpid(), *cur_letter);
        if (*cur_letter == 'z')
            (*cur_letter) -= 26;
        (*cur_letter)++;
        (*prod_pos)++;
        if (semop(semid, stop_produce, 2) == -1)
        {
            perror("semop stop produce\n");
            exit(1);
        }
    }
    if (shmdt((void *)addr) == -1)
    {
        perror("shmdt\n");
        exit(1);
    }
    exit(0);
}

void consumer(const int semid, const int shmid)
{
    char *addr = (char *)shmat(shmid, 0, 0);
    if (addr == (char *)-1)
    {
        perror("shmat\n");
        exit(1);
    }
    char **prod_pos = (char **)addr;
    char **cons_pos = prod_pos + sizeof(char);
    char *cur_letter = (char *)(cons_pos + sizeof(char));
    srand(time(NULL));
    while (flag)
    {
        sleep(rand() % 4 + 1);
        if (semop(semid, start_consume, 2) == -1)
        {
            perror("semop start consume\n");
            exit(1);
        }
        printf("Consumer %d -> %c\n", getpid(), **cons_pos);
        (*cons_pos)++;
        if (semop(semid, stop_consume, 2) == -1)
        {
            perror("semop stop consume\n");
            exit(1);
        }
    }
    if (shmdt((void *)addr) == -1)
    {
        perror("shmdt\n");
        exit(1);
    }
    exit(0);
}

int main()
{
    if (signal(SIGINT, sig_handler) == -1)
    {
        perror("signal\n");
        exit(1);
    }
    int shmid, semid;
    /* S_IRUSR - владелец может читать
     * S_IRUSR - владелец может писать
     * S_IRGRP - группа может читать
     * S_IROTH - остальные могут читать
     */
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    char **prod_pos;
    char **cons_pos;
    char *cur_letter;

    key_t shmkey = ftok("./keyk.txt", 1);
    if (shmkey == -1)
    {
        perror("ftok\n");
        exit(1);
    }

    if ((shmid = shmget(shmkey, 1024, IPC_CREAT | perms)) == -1)
    {
        perror("shmget\n");
        exit(1);
    }
    char *buf = shmat(shmid, NULL, 0);
    if (buf == (char *)-1)
    {
        perror("shmat\n");
        exit(1);
    }

    prod_pos = (char **)buf;
    cons_pos = prod_pos + 1;
    cur_letter = (char *)(cons_pos + 1);
    *cons_pos = cur_letter + 1;
    *prod_pos = *cons_pos;
    *cur_letter = 'a';

    key_t semkey = ftok("./keyk.txt", 1);
    if (semkey == -1)
    {
        perror("ftok\n");
        exit(1);
    }

    if ((semid = semget(semkey, 3, IPC_CREAT | perms)) == -1)
    {
        perror("semget\n");
        exit(1);
    }

    int sem_empty = semctl(semid, SEM_EMPTY, SETVAL, SIZE);
    int sem_binary = semctl(semid, SEM_BINARY, SETVAL, 1);
    int sem_full = semctl(semid, SEM_FULL, SETVAL, 0);

    if (sem_empty == -1 || sem_binary == -1 || sem_full == -1)
    {
        perror("semctl\n");
        exit(1);
    }

    pid_t chpid[N_CONS + N_PROD - 1];
    for (int i = 0; i < N_PROD - 1; i++)
    {
        chpid[i] = fork();
        if (chpid[i] == -1)
        {
            perror("fork\n");
            exit(1);
        }
        if (chpid[i] == 0)
            producer(semid, shmid);
    }
    for (int i = N_PROD - 1; i < N_CONS + N_PROD - 1; i++)
    {
        chpid[i] = fork();
        if (chpid[i] == -1)
        {
            perror("fork\n");
            exit(1);
        }
        if (chpid[i] == 0)
            consumer(semid, shmid);
    }

    srand(time(NULL));

        while (flag)
    {
        sleep(rand() % 2 + 1);
        if (semop(semid, start_produce, 2) == -1)
        {
            perror("semop start produce\n");
            exit(1);
        }
        **prod_pos = *cur_letter;
        printf("Producer %d -> %c\n", getpid(), *cur_letter);
        if (*cur_letter == 'z')
            (*cur_letter) -= 26;
        (*cur_letter)++;
        (*prod_pos)++;
        if (semop(semid, stop_produce, 2) == -1)
        {
            perror("semop stop produce\n");
            exit(1);
        }
    }

    for (int i = 0; i < N_PROD + N_CONS - 1; i++)
    {
        int status;
        if (waitpid(chpid[i], &status, WUNTRACED) == -1)
        {
            perror("waitpid\n");
            exit(1);
        }

        if (WIFEXITED(status))
            printf("%d exited, status = %d\n", chpid[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("%d killed by signal %d\n", chpid[i], WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("%d stopped by signal %d\n", chpid[i], WSTOPSIG(status));
    }

    if (shmdt((void *)prod_pos) == -1)
    {
        perror("shmdt\n");
        exit(1);
    }

    if (semctl(semid, 1, IPC_RMID, NULL) == -1)
    {
        perror("semctl\n");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl\n");
        exit(1);
    }
    return 0;
}