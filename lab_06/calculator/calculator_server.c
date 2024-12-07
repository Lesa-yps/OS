/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "calculator.h"

struct CALCULATOR *
calculator_proc_1_svc(struct CALCULATOR *argp, struct svc_req *rqstp)
{
	static struct CALCULATOR  result;

	/*
	 * insert server code here
	 */

	/* -<<< Add to test */
	switch(argp->op)
	{
		case ADD:
			result.result = argp->arg1 + argp->arg2;
			break;
		case SUB:
			result.result = argp->arg1 - argp->arg2;
			break;
		case MUL:
			result.result = argp->arg1 * argp->arg2;
			break;
		case DIV:
			result.result = argp->arg1 / argp->arg2;
			break;
		default:
			break;
	}
	printf("Ко мне обратился клиент c запросом %.2f %s %.2f = %.2f\n", 
           argp->arg1, argp->op == 0 ? "+" : argp->op == 1 ? "-" : argp->op == 2 ? "*" : "/", argp->arg2, result.result);
	/* -<<< Add to test */

	return &result;
}