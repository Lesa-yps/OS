/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "bakery.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_CLIENTS 1024

int cur = 0;

static bool choosing[MAX_CLIENTS] = { false };
static int numbers[MAX_CLIENTS] = { 0 };
static int pids[MAX_CLIENTS] = { 0 };
static int max_n = 0;


int find_max_number()
{
    for (int i = 1; i < MAX_CLIENTS; i++)
        if (numbers[i] > max_n)
            max_n = numbers[i];
    max_n++;
    return max_n;
}


struct REQUEST *get_number_1_svc(struct REQUEST *argp, struct svc_req *rqstp)
{
	static struct REQUEST  result;

	result.index = cur;
    cur++;
    cur %= MAX_CLIENTS;

    pids[result.index] = argp->pid;

    choosing[result.index] = true;
    int next = find_max_number();
    result.number = next;
    numbers[result.index] = next;
    choosing[result.index] = false;

    printf("Client (pid %d) receive number %d\n", argp->pid, result.number);

	return &result;
}


float *bakery_service_1_svc(struct REQUEST *argp, struct svc_req *rqstp)
{
	static float  result;

	for (int i = 0; i < MAX_CLIENTS; i++)
    {
        while (choosing[i])
            ;
        while (numbers[i] != 0 && ((numbers[i] < numbers[argp->index]) || (numbers[i] == numbers[argp->index] && pids[i] < pids[argp->index])))
            ;
    }

    // calculate
    int c_op = '!';
    switch (argp->op)
    {
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

    numbers[argp->index] = 0;

    printf("Client with number %d (pid: %d): %.2f %c %.2f = %.2f\n", argp->number, argp->pid, argp->arg1, c_op, argp->arg2, result);

	return &result;
}