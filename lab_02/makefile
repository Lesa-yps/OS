CC = gcc

task_1: task_1.c
	$(CC) $<
	./a.out

task_2: task_2.c
	$(CC) $<
	./a.out

task_3: task_3.c sum.c two_degree.c
	$(CC) sum.c -lm
	$(CC) two_degree.c -lm
	$(CC) $<
	./a.out

task_4: task_4.c
	$(CC) $<
	./a.out

task_5: task_5.c
	$(CC) $<
	./a.out