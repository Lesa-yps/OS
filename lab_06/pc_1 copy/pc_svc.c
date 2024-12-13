#include "pc.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

pthread_t thread;
pthread_attr_t attr;
int semid;

// Структура данных для передачи в поток
struct thread_data {
    struct svc_req *rqstp;
    SVCXPRT *transp;
};

// Функция для обработки запроса в отдельном потоке
void *process_request(void *data) {
    struct thread_data *td = (struct thread_data *)data;

    struct svc_req *rqstp = td->rqstp;
    register SVCXPRT *transp = td->transp;

    union {
        struct PROD_CONS service_1_arg;
    } argument;
    union {
        struct PROD_CONS service_1_res;
    } result;
    bool_t retval;
    xdrproc_t _xdr_argument, _xdr_result;
    bool_t (*local)(char *, void *, struct svc_req *);

    switch (rqstp->rq_proc) {
    case NULLPROC:
        (void) svc_sendreply(transp, (xdrproc_t) xdr_void, (char *)NULL);
        free(td);  // Освобождаем данные
        return NULL;

    case SERVICE:
        _xdr_argument = (xdrproc_t) xdr_PROD_CONS;
        _xdr_result = (xdrproc_t) xdr_PROD_CONS;
        local = (bool_t (*)(char *, void *, struct svc_req *))service_1_svc;
        break;

    default:
        svcerr_noproc(transp);
        free(td);  // Освобождаем данные
        return NULL;
    }

    memset((char *)&argument, 0, sizeof(argument));
    if (!svc_getargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        svcerr_decode(transp);
        free(td);  // Освобождаем данные
        return NULL;
    }

    retval = (bool_t)(*local)((char *)&argument, (void *)&result, rqstp);
    if (retval > 0 && !svc_sendreply(transp, (xdrproc_t)_xdr_result, (char *)&result)) {
        svcerr_systemerr(transp);
    }

    if (!svc_freeargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        fprintf(stderr, "%s", "unable to free arguments");
        exit(1);
    }

    if (!pc_prog_1_freeresult(transp, _xdr_result, (caddr_t)&result)) {
        fprintf(stderr, "%s", "unable to free results");
    }

    free(td);  // Освобождаем данные
    return NULL;
}

// Модифицированная функция для обработки соединений
static void pc_prog_1(struct svc_req *rqstp, register SVCXPRT *transp) {
    struct thread_data *td = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (td == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    td->rqstp = rqstp;
    td->transp = transp;

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attr, process_request, (void *)td) != 0) {
        fprintf(stderr, "Thread creation error\n");
        free(td);
    }
}

int main(int argc, char **argv) {
    register SVCXPRT *transp;

    // Инициализация семафоров
    key_t semkey = ftok("file.txt", 4);
    if (semkey == (key_t)-1) {
        perror("Error: ftok\n");
        exit(1);
    }

    semid = semget(semkey, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Error: semget\n");
        exit(1);
    }

    // Инициализация атрибутов потока
    pthread_attr_init(&attr);

    pmap_unset(PC_PROG, PC_VER);

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        fprintf(stderr, "%s", "cannot create udp service.\n");
        exit(1);
    }
    if (!svc_register(transp, PC_PROG, PC_VER, pc_prog_1, IPPROTO_UDP)) {
        fprintf(stderr, "%s", "unable to register (PC_PROG, PC_VER, udp).\n");
        exit(1);
    }

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        fprintf(stderr, "%s", "cannot create tcp service.\n");
        exit(1);
    }
    if (!svc_register(transp, PC_PROG, PC_VER, pc_prog_1, IPPROTO_TCP)) {
        fprintf(stderr, "%s", "unable to register (PC_PROG, PC_VER, tcp).\n");
        exit(1);
    }

    svc_run();
    fprintf(stderr, "%s", "svc_run returned\n");
    exit(1);
    /* NOTREACHED */
}
