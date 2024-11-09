//the program calculates the movement according
// to the entered initial speed, acceleration and time


//header file connection
#include <stdio.h>
#include <unistd.h>

//macro declaration OK
#define OK 0
#define ERROR 1

//main function
int main(void)
{
	//real variables for initial speed, acceleration and time
	double v0, a, t;
	//real variable to move
	double s;
	//input of initial speed, acceleration and time
	printf("pid = %d Input v0, a, t:\n", getpid());
	if (scanf("%lf%lf%lf", &v0, &a, &t) != 3)
	{
		printf("Input error.\n");
		return ERROR;
	}
	else
	{
		//move calculation
		s = v0 * t + a * t * t / 2;

		//move output
		printf("pid = %d Result = %lf\n", getpid(), s);
		return OK;
	}
}