#include <stdio.h>
#include <math.h>
#include <unistd.h>

int main(void)
{
	int degree;
	printf("Child: pid = %d. Input degree for 2:\n", getpid());
	if (scanf("%d", &degree) == 1)
	{
		int res = pow(2, degree);
		printf("Child: pid = %d. Result 2^%d = %d\n", getpid(), degree, res);
	}
	else
	{
		printf("Error with input two degree\n");
	}
    return 0;
}