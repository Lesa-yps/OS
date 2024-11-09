//the program calculates the area of a triangle given
// two sides and the angle between them


//header files connection
#include <stdio.h>
#include <math.h>
#include <unistd.h>

//macro declarations
#define OK 0
#define ERROR 1
#define PI 3.1415926535

//main function
int main(void)
{
	//real variables for sides and angle
	long double a, b, alpha;
	//real variables for area
	long double s;
	//enter sides and angle
	printf("pid = %d Input a, b, alpha:\n", getpid());
	if (scanf("%Lf%Lf%Lf", &a, &b, &alpha) != 3)
	{
		printf("Input error.\n");
		return ERROR;
	}
	else
	{
		//convert angle to radians
		alpha *= PI / 180.0;
		//area calculation
		s = a * b * sin(alpha) / 2.0;

		//output area
		printf("pid = %d Result = %Lf\n", getpid(), s);
		return OK;
	}
}