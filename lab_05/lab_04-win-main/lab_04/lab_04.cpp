// Монитор Хоара «Читатели-писатели» в Windows
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <stdint.h>

// кол-во читателей и писателей
#define WRIT_CNT 4
#define READ_CNT 5

// Мьютекс
HANDLE mutex_num;
// Событие для разрешения чтения
HANDLE can_read;
// Событие для разрешения записи     
HANDLE can_write;

// Переменные для отслеживания состояния (счётчика)
volatile LONG write_queue = 0;
volatile LONG read_queue = 0;
volatile LONG active_readers = 0;

// глобальная/разделяемая переменная
volatile int num = 0;
// флаг о получении сигнала о завершении
volatile int sig_flag = 1;

// читатель может начать читать, если нет пишущего писателя или нет ждущих писателей
void start_read(void)
{
	// увеличиваем количество ожидающих читателей
	InterlockedIncrement(&read_queue);

	// Если есть пишущий писатель или ждущие писатели, ждем разрешения на чтение
	if (write_queue || WaitForSingleObject(can_write, 0) == WAIT_OBJECT_0)
		WaitForSingleObject(can_read, INFINITE);

	WaitForSingleObject(mutex_num, INFINITE);

	// Уменьшаем количество ожидающих читателей и увеличиваем количество активных читателей
	InterlockedDecrement(&read_queue);
	InterlockedIncrement(&active_readers);

	// Разрешаем чтение
	SetEvent(can_read);
	// Освобождаем мьютекс
	ReleaseMutex(mutex_num);
}

void stop_read(void)
{
	// Уменьшаем количество активных читателей
	InterlockedDecrement(&active_readers);

	// Если нет активных читателей, разрешаем запись
	if (active_readers == 0)
		SetEvent(can_write);
}

// писатель может писать, если нет активных читателей и нет активных писателей
void start_write(void)
{
	// Увеличиваем количество ожидающих писателей
	InterlockedIncrement(&write_queue);

	// Если есть активный читатель или писатель, ждем разрешения на запись
	if (WaitForSingleObject(can_read, 0) == WAIT_OBJECT_0)
		WaitForSingleObject(can_write, INFINITE);
	WaitForSingleObject(mutex_num, INFINITE);
	// Уменьшаем количество ожидающих писателей
	InterlockedDecrement(&write_queue);
	ReleaseMutex(mutex_num);
}

void stop_write(void)
{
	ResetEvent(can_write);
	// Если есть ожидающие читатели, разрешаем им читать
	if (read_queue > 0)
		SetEvent(can_read);
	else
		SetEvent(can_write); // Иначе, разрешаем запись
}

// ПИСАТЕЛЬ
DWORD WINAPI func_writer(CONST LPVOID lpParams)
{
	srand(time(NULL));
	// Идентификатор читателя
	intptr_t writer_id = (intptr_t)lpParams;
	while (sig_flag)
	{
		Sleep((rand() % 3 + 2) * 1000);
		// Ожидание разрешения на запись
		start_write();
		// Запись нового значения
		num++;
		if (num > 25)
			num = 0;
		printf("Writer %lu -> %c\n", GetCurrentThreadId(), 'a' + num);
		// Завершаем запись
		stop_write();
	}
	return 0;
}

// ЧИТАТЕЛЬ
DWORD WINAPI func_reader(CONST LPVOID lpParams)
{
	srand(time(NULL));
	// Идентификатор читателя
	intptr_t reader_id = (intptr_t)lpParams;
	while (sig_flag)
	{
		Sleep((rand() % 2 + 2) * 1000);
		// Ожидание разрешения на чтение
		start_read();
		printf("Reader %lu -> %c\n", GetCurrentThreadId(), 'a' + num);
		// Завершаем чтение
		stop_read();
	}
	return 0;
}

// Устанавливаем флаг при получении сигнала
BOOL WINAPI signal_handler(DWORD signal)
{
	if (signal == CTRL_C_EVENT)
	{
		sig_flag = 0;
		printf("Program catch Ctrl-C.\n");
		return TRUE;
	}
	return FALSE;
}

int main(void)
{
	setbuf(stdout, NULL);

	// Устанавливаем обработчик сигнала
	if (!SetConsoleCtrlHandler(signal_handler, TRUE))
	{
		perror("Error: set signal handler.\n");
		exit(EXIT_FAILURE);
	}

	// Массив для дескрипторов потоков
	HANDLE pthread[WRIT_CNT + READ_CNT];
	DWORD thid[WRIT_CNT + READ_CNT];

	// Создаем мьютекс для синхронизации доступа к общей переменной
	mutex_num = CreateMutex(NULL, FALSE, NULL);
	if (mutex_num == NULL)
	{
		perror("Error: CreateMutex");
		return 1;
	}

	// Создаем события для синхронизации чтения и записи
	can_read = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (can_read == NULL)
	{
		perror("Error: CreateEvent");
		return 1;
	}
	can_write = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (can_write == NULL)
	{
		perror("Error: CreateEvent");
		return 1;
	}

	// ЗАПУСКАЕМ ПРОЦЕССЫ-ПИСАТЕЛИ
	for (int i = 0; i < WRIT_CNT; i++)
	{
		pthread[i] = CreateThread(NULL, 0, func_writer, (LPVOID)(intptr_t)i, 0, &thid[i]);
		if (pthread[i] == NULL)
		{
			DWORD errorCode = GetLastError();
			printf("Error: CreateThread. Error code: %lu\n", errorCode);
			return 1;
		}
	}

	// ЗАПУСКАЕМ ПРОЦЕССЫ-ЧИТАТЕЛИ
	for (int i = WRIT_CNT; i < WRIT_CNT + READ_CNT; i++)
	{
		pthread[i] = CreateThread(NULL, 0, func_reader, (LPVOID)(intptr_t)i, 0, &thid[i]);
		if (pthread[i] == NULL)
		{
			DWORD errorCode = GetLastError();
			printf("Error: CreateThread. Error code: %lu\n", errorCode);
			return 1;
		}
	}

	for (int i = 0; i < WRIT_CNT + READ_CNT; i++)
	{
		DWORD dw = WaitForSingleObject(pthread[i], INFINITE);
		switch (dw)
		{
		case WAIT_OBJECT_0:
			printf("thread %d finished\n", thid[i]);
			break;
		case WAIT_TIMEOUT:
			printf("waitThread timeout %d\n", dw);
			break;
		case WAIT_FAILED:
			printf("waitThread failed %d\n", dw);
			break;
		default:
			printf("unknown %d\n", dw);
			break;
		}
	}

	// Закрываем все дескрипторы
	CloseHandle(mutex_num);
	CloseHandle(can_read);
	CloseHandle(can_write);

	return 0;
}