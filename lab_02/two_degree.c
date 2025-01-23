#include <stdio.h>
#include <math.h>

int main(void)
{
	int degree;
	printf("Input degree for 2:\n");
	if (scanf("%d", &degree) == 1)
	{
		float res = pow(2, degree);
		printf("Result 2^%d = %f\n", degree, res);
	}
	else
	{
		printf("Error with input sum\n");
	}
    return 0;
}