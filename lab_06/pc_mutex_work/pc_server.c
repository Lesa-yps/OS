#include "pc.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define SIZE_BUF 26

// Глобальные данные
static char buffer[SIZE_BUF];
static char *addr_prod = buffer;
static char *addr_cons = buffer;
static char letter = 'a';

// Мьютекс для синхронизации
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char producer(void)
{
    pthread_mutex_lock(&mutex);

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

    pthread_mutex_unlock(&mutex);
    return produced;
}

char consumer(void)
{
    pthread_mutex_lock(&mutex);

    char consumed = *addr_cons;
    printf("Consumer -> %c\n", consumed);

    if (addr_cons == buffer + SIZE_BUF - 1) {
        addr_cons = buffer;
    } else {
        addr_cons++;
    }

    pthread_mutex_unlock(&mutex);
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
