// ������� ����� ���������-�������� � Windows
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <stdint.h>

// ���-�� ��������� � ���������
#define WRIT_CNT 4
#define READ_CNT 5

// �������
HANDLE mutex_num;
// ������� ��� ���������� ������
HANDLE can_read;
// ������� ��� ���������� ������     
HANDLE can_write;

// ���������� ��� ������������ ��������� (��������)
volatile LONG write_queue = 0;
volatile LONG read_queue = 0;
volatile LONG active_readers = 0;

// ����������/����������� ����������
volatile int num = 0;
// ���� � ��������� ������� � ����������
volatile int sig_flag = 1;

// �������� ����� ������ ������, ���� ��� �������� �������� ��� ��� ������ ���������
void start_read(void)
{
	// ����������� ���������� ��������� ���������
	InterlockedIncrement(&read_queue);

	// ���� ���� ������� �������� ��� ������ ��������, ���� ���������� �� ������
	if (write_queue || WaitForSingleObject(can_write, 0) == WAIT_OBJECT_0)
		WaitForSingleObject(can_read, INFINITE);

	WaitForSingleObject(mutex_num, INFINITE);

	// ��������� ���������� ��������� ��������� � ����������� ���������� �������� ���������
	InterlockedDecrement(&read_queue);
	InterlockedIncrement(&active_readers);

	// ��������� ������
	SetEvent(can_read);
	// ����������� �������
	ReleaseMutex(mutex_num);
}

void stop_read(void)
{
	// ��������� ���������� �������� ���������
	InterlockedDecrement(&active_readers);

	// ���� ��� �������� ���������, ��������� ������
	if (active_readers == 0)
		SetEvent(can_write);
}

// �������� ����� ������, ���� ��� �������� ��������� � ��� �������� ���������
void start_write(void)
{
	// ����������� ���������� ��������� ���������
	InterlockedIncrement(&write_queue);

	// ���� ���� �������� �������� ��� ��������, ���� ���������� �� ������
	if (WaitForSingleObject(can_read, 0) == WAIT_OBJECT_0)
		WaitForSingleObject(can_write, INFINITE);
	WaitForSingleObject(mutex_num, INFINITE);
	// ��������� ���������� ��������� ���������
	InterlockedDecrement(&write_queue);
	ReleaseMutex(mutex_num);
}

void stop_write(void)
{
	ResetEvent(can_write);
	// ���� ���� ��������� ��������, ��������� �� ������
	if (read_queue > 0)
		SetEvent(can_read);
	else
		SetEvent(can_write); // �����, ��������� ������
}

// ��������
DWORD WINAPI func_writer(CONST LPVOID lpParams)
{
	srand(time(NULL));
	// ������������� ��������
	intptr_t writer_id = (intptr_t)lpParams;
	while (sig_flag)
	{
		Sleep((rand() % 3 + 2) * 1000);
		// �������� ���������� �� ������
		start_write();
		// ������ ������ ��������
		num++;
		if (num > 25)
			num = 0;
		printf("Writer %lu -> %c\n", GetCurrentThreadId(), 'a' + num);
		// ��������� ������
		stop_write();
	}
	return 0;
}

// ��������
DWORD WINAPI func_reader(CONST LPVOID lpParams)
{
	srand(time(NULL));
	// ������������� ��������
	intptr_t reader_id = (intptr_t)lpParams;
	while (sig_flag)
	{
		Sleep((rand() % 2 + 2) * 1000);
		// �������� ���������� �� ������
		start_read();
		printf("Reader %lu -> %c\n", GetCurrentThreadId(), 'a' + num);
		// ��������� ������
		stop_read();
	}
	return 0;
}

// ������������� ���� ��� ��������� �������
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

	// ������������� ���������� �������
	if (!SetConsoleCtrlHandler(signal_handler, TRUE))
	{
		perror("Error: set signal handler.\n");
		exit(EXIT_FAILURE);
	}

	// ������ ��� ������������ �������
	HANDLE pthread[WRIT_CNT + READ_CNT];
	DWORD thid[WRIT_CNT + READ_CNT];

	// ������� ������� ��� ������������� ������� � ����� ����������
	mutex_num = CreateMutex(NULL, FALSE, NULL);
	if (mutex_num == NULL)
	{
		perror("Error: CreateMutex");
		return 1;
	}

	// ������� ������� ��� ������������� ������ � ������
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

	// ��������� ��������-��������
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

	// ��������� ��������-��������
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

	// ��������� ��� �����������
	CloseHandle(mutex_num);
	CloseHandle(can_read);
	CloseHandle(can_write);

	return 0;
}