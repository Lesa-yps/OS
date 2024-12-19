/*
 * Этот код сгенерирован rpcgen как пример.
 * Он может служить шаблоном для разработки собственных функций.
 */

#include "bakery.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_CLIENTS 1024 /* Максимальное количество клиентов */

int cur = 0; /* Текущий индекс для обработки запросов */

/* Массивы для отслеживания состояния клиентов */
static bool choosing[MAX_CLIENTS] = { false }; /* Флаг выбора номера клиентом */
static int numbers[MAX_CLIENTS] = { 0 };      /* Присвоенные номера клиентов */
static int pids[MAX_CLIENTS] = { 0 };         /* PID процессов клиентов */
static int max_n = 0;                         /* Максимальный номер в системе */

/* 
 * Функция для определения следующего максимального номера.
 * Используется для присвоения уникальных номеров клиентам.
 */
int max_number()
{
    for (int i = 1; i < MAX_CLIENTS; i++)
        if (numbers[i] > max_n)
            max_n = numbers[i];
    max_n++;
    return max_n;
}

/* 
 * Реализация удаленной процедуры get_number.
 * Эта функция присваивает клиенту уникальный номер.
 */
struct REQUEST *get_number_1_svc(struct REQUEST *argp, struct svc_req *rqstp)
{
    static struct REQUEST result;

    /* Присвоение индекса клиенту */
    argp->index = result.index = cur;
    cur++;
    cur %= MAX_CLIENTS;

    /* Сохранение PID клиента */
    pids[argp->index] = argp->pid;

    /* Начало выбора номера */
    choosing[argp->index] = true;
    int next = max_number(); /* Получение нового уникального номера */
    result.number = next;
    numbers[argp->index] = next; /* Сохранение номера для клиента */
    choosing[argp->index] = false; /* Завершение выбора номера */

    printf("Client (pid %d) receive number %d\n", argp->pid, result.number);

    return &result;
}

/* 
 * Реализация удаленной процедуры bakery_service.
 * Эта функция выполняет указанную клиентом арифметическую операцию.
 */
float *bakery_service_1_svc(struct REQUEST *argp, struct svc_req *rqstp)
{
    static float result;

    /* Ожидание завершения выбора номера другими клиентами */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        while (choosing[i]) /* Ожидание, пока клиент завершит выбор номера */
            ;
        /* Проверка приоритетов по номерам и PID */
        while (numbers[i] != 0 && (numbers[i] < numbers[argp->index] || 
               (numbers[i] == numbers[argp->index] && pids[i] < pids[argp->index])))
            ;
    }

    /* Выполнение арифметической операции */
    int c_op = '!'; /* Символ операции (для вывода в лог) */
    switch (argp->op) {
    case ADD:
        result = argp->arg1 + argp->arg2;
        c_op = '+';
        break;
    case SUB:
        result = argp->arg1 - argp->arg2;
        c_op = '-';
        break;
    case MUL:
        result = argp->arg1 * argp->arg2;
        c_op = '*';
        break;
    case DIV:
        result = argp->arg1 / argp->arg2;
        c_op = '/';
        break;
    default:
        break;
    }

    /* Освобождение номера клиента */
    numbers[argp->index] = 0;

    /* Логирование результата */
    printf("Client with number %d (pid: %d): %.2f %c %.2f = %.2f\n", 
           argp->number, argp->pid, argp->arg1, c_op, argp->arg2, result);

    return &result;
}
