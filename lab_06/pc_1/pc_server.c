#include "pc.h"
#include <pthread.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


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


// Глобальные данные
static char buffer[SIZE_BUF];
static char *addr_prod = buffer;
static char *addr_cons = buffer;
static char letter = 'a';


char producer(void)
{
    // изменение значений 2-ух семафоров (start_produce)
    if (semop(semid, start_produce, 2) == -1)
    {
        char err_msg[100];
        sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
        perror(err_msg);
        exit(1);
    }

	// записываем букву в буфер и выводим её
    *addr_prod = letter;
    printf("Producer -> %c\n", *addr_prod);
    char produced = *addr_prod;

    if (letter == 'z') {
        addr_prod = buffer;
        letter = 'a';
    } else {
        addr_prod++;
        letter++;
    }

    // изменение значений 2-ух семафоров (stop_produce)
    if (semop(semid, stop_produce, 2) == -1)
    {
        char err_msg[100];
        sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
        perror(err_msg);
        exit(1);
    }
    return produced;
}

char consumer(void)
{
    // изменение значений 2-ух семафоров (start_consume)
    if (semop(semid, start_consume, 2) == -1)
    {
        char err_msg[100];
        sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
        perror(err_msg);
        exit(1);
    }

    // выводим букву из буфера
    char consumed = *addr_cons;
    printf("Consumer -> %c\n", consumed);

    if (addr_cons == buffer + SIZE_BUF - 1)
        addr_cons = buffer;
    else
        addr_cons++;

	// изменение значений 2-ух семафоров (stop_consume)
    if (semop(semid, stop_consume, 2) == -1)
    {
        char err_msg[100];
        sprintf(err_msg, "Error: semop pid = %d, errno %d", getpid(), errno);
        perror(err_msg);
        exit(1);
    }
    return consumed;
}

bool_t
service_1_svc(struct PROD_CONS *argp, struct PROD_CONS *result, struct svc_req *rqstp)
{
    // Проверка аргумента
    if (!argp || !result)
        return FALSE;

    if (argp->type == 0) {
        result->result = producer();
    } else if (argp->type == 1) {
        result->result = consumer();
    } else {
        return FALSE; // Неверный аргумент
    }

    return TRUE;
}

int
pc_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    xdr_free(xdr_result, result);
    return 1;
}
