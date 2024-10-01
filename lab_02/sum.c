#include <stdio.h>

int main(void)
{
	int a, b;
	printf("Input two numbers for sum:\n");
	if (scanf("%d%d", &a, &b) == 2)
	{
		int c = a + b;
		printf("Result %d + %d = %d\n", a, b, c);
	}
	else
	{
		printf("Error with input sum\n");
	}
    return 0;
}