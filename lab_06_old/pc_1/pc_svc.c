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
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

pthread_t thread;
pthread_attr_t attr;
int semid;
char **ptr_prod;
char **ptr_cons;
char *letter;

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

    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t shmkey = ftok("file.txt", 1);
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
    char *letter = (char*) (ptr_cons + sizeof(char)); // текущая буква
    // установка стартовых значений
    *ptr_cons = letter + sizeof(char); // потребитель начинает с первой ячейки буфера
    *ptr_prod = *ptr_cons; // производитель начинает с первой ячейки буфера
    *letter = 'a'; // первая буква 'a'
    // ftok - преобразовывает имя файла и идентификатор проекта в ключ для системных вызовов
    key_t semkey = ftok("file.txt", 2);
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
