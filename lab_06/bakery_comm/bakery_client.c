/*
 * Этот код сгенерирован rpcgen как пример.
 * Он может служить шаблоном для разработки собственных функций.
 */

#include "bakery.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * Функция bakery_prog_1 выполняет взаимодействие клиента с сервером.
 * Она отправляет запросы на получение уникального номера и выполнение
 * арифметической операции, а также обрабатывает ответы.
 */
void bakery_prog_1(char *host)
{
	CLIENT *clnt; /* Указатель на клиентскую структуру RPC */
	struct REQUEST  *result_1; /* Указатель на результат от сервера (номер) */
	struct REQUEST request;   /* Структура для передачи данных на сервер */
    request.pid = getpid();   /* Установка PID процесса клиента */
	float  *result_2;         /* Указатель на результат арифметической операции */

#ifndef	DEBUG
	/* Создание клиентской структуры RPC */
	clnt = clnt_create (host, BAKERY_PROG, BAKERY_VER, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host); /* Ошибка создания клиента */
		exit (1);
	}
#endif	/* DEBUG */

	/* Инициализация генератора случайных чисел */
	srand(time(NULL));

    /* Задержка перед запросом на получение номера */
    usleep(rand() % 1000 + 5000000);

	/* Вызов удаленной процедуры для получения уникального номера */
	result_1 = get_number_1(&request, clnt);
	if (result_1 == (struct REQUEST *) NULL) {
		clnt_perror (clnt, "call failed"); /* Ошибка вызова RPC */
	}

	/* Сохранение полученного номера и индекса */
	request.index = result_1->index;
    request.number = result_1->number;

	printf("Client (pid: %d) receive number: %d\n", request.pid, request.number);

    /* Задержка перед выполнением арифметической операции */
    usleep(rand() % 1000 + 5000000);

	/* Генерация случайной операции и аргументов */
	request.op = rand() % 4; /* Операция: ADD, SUB, MUL, DIV */
    request.arg1 = rand() % 1000 + 1; /* Первый аргумент */
    request.arg2 = rand() % 900 + 1;  /* Второй аргумент */

	/* Вызов удаленной процедуры для выполнения операции */
	result_2 = bakery_service_1(&request, clnt);
	if (result_2 == (float *) NULL) {
		clnt_perror (clnt, "call failed"); /* Ошибка вызова RPC */
	}

	/* Определение символа операции для вывода */
	char c_op = '!';
    switch (request.op) {
    case ADD:
        c_op = '+';
        break;
    case SUB:
        c_op = '-';
        break;
    case MUL:
        c_op = '*';
        break;
    case DIV:
        c_op = '/';
        break;
    default:
        break;
    }

    /* Вывод результата операции */
    printf("Client with number %d (pid: %d): %.2f %c %.2f = %.2f\n", 
           request.number, request.pid, request.arg1, c_op, request.arg2, *result_2);

#ifndef	DEBUG
	/* Уничтожение клиентской структуры */
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}

/*
 * Основная функция main.
 * Ожидает аргумент с адресом сервера и вызывает bakery_prog_1.
 */
int main (int argc, char *argv[])
{
	char *host;

	/* Проверка аргументов командной строки */
	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}

	/* Сохранение имени или IP-адреса сервера */
	host = argv[1];
	
	/* Запуск в бесконечном цикле клиентской программы */
	while (1)
        bakery_prog_1(host);

	exit (0); /* Завершение работы программы */
}
