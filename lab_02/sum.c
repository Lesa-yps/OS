#include <stdio.h>
#include <unistd.h>

int main(void)
{
	int a, b;
	printf("Child: pid = %d. Input two numbers for sum:\n", getpid());
	if (! scanf("%d%d", &a, &b) == 2)
	{
		printf("Error with input sum\n");
		return 1;
	}
	int c = a + b;
	printf("Child: pid = %d. Result %d + %d = %d\n", getpid(), a, b, c);
    return 0;
}