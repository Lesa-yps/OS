/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "bakery.h"
#include <time.h>


void
bakery_prog_1(char *host)
{
	CLIENT *clnt;
	char *get_number_1_arg;
	struct REQUEST request;
	float *result_2;
	int *result_1;

#ifndef	DEBUG
	clnt = clnt_create (host, BAKERY_PROG, BAKERY_VER, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */

	srand(time(NULL));
    usleep(rand() % 1000 + 5000000);

	result_1 = get_number_1((void*)&get_number_1_arg, clnt);
	if (result_1 == (int *) NULL) {
		clnt_perror (clnt, "call failed");
	}
	request.number = *result_1;

	printf("Client receive number: %d\n", request.number);

    usleep(rand() % 1000 + 5000000);

	request.op = rand() % 4;
    request.arg1 = rand() % 1000 + 1;
    request.arg2 = rand() % 900 + 1;

	result_2 = bakery_service_1(&request, clnt);
	if (result_2 == (float *) NULL)
		clnt_perror (clnt, "call failed");

	char c_op;
    switch (request.op)
	{
		case ADD:
			c_op = '+';
			break;
		case SUB:
			c_op = '-';
			break;
		case MUL:
			c_op = '*';
			break;
		default:
			c_op = '/';
    }

    printf("Client with number %d: %.2f %c %.2f = %.2f\n", request.number, request.arg1, c_op, request.arg2, *result_2);
#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}


int
main (int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	while (1)
		bakery_prog_1 (host);
exit (0);
}
