// Монитор Хоара «Читатели-писатели» в Windows
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <stdint.h>

// кол-во читателей и писателей
#define WRIT_CNT 4
#define READ_CNT 5

// Мьютекс для защиты общей переменной (num)
HANDLE mutex_num;        
// Событие для разрешения чтения
HANDLE can_read;
// Событие для разрешения записи     
HANDLE can_write;

// Переменные для отслеживания состояния (счётчика)
int write_queue = 0;
int read_queue = 0;
int active_readers = 0;
int active_writer = 0;

// глобальная/разделяемая переменная
int num = 0;

// флаг о получении сигнала о завершении
int sig_flag = 1;


// читатель может начать читать, если нет пишущего писателя или нет ждущих писателей
void start_read(void)
{
    // увеличиваем количество ожидающих читателей
    InterlockedIncrement(&read_queue);

    // Если есть пишущий писатель или ждущие писатели, ждем разрешения на чтение
    if (sig_flag && (active_writer || (WaitForSingleObject(can_write, 0) == WAIT_OBJECT_0 && write_queue)))
        WaitForSingleObject(can_read, INFINITE);
    if (! sig_flag)
        return;
    
    // Блокируем доступ к общей переменной
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
    {
        ResetEvent(can_read);
        SetEvent(can_write);
    }
}

// писатель может писать, если нет активных читателей и нет активных писателей
void start_write(void)
{
    // Увеличиваем количество ожидающих писателей
    InterlockedIncrement(&write_queue);

    // Если есть активный читатель или писатель, ждем разрешения на запись
    if (sig_flag && (active_writer || (active_readers > 0)))
        WaitForSingleObject(can_write, INFINITE);
    if (! sig_flag)
        return;

    // Уменьшаем количество ожидающих писателей
    InterlockedDecrement(&write_queue);

    // Устанавливаем флаг активного писателя
    active_writer = 1;
}

void stop_write(void)
{
    // Снимаем флаг активного писателя
    active_writer = 0;

    // Если есть ожидающие читатели, разрешаем им читать
    if (read_queue)
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
        Sleep((rand() % 4 + 1) * 1000);
        // Ожидание разрешения на запись
        start_write();
        if (!sig_flag)
            break;
        // Запись нового значения
        num++;
        printf("Writer %ld inc -> %d\n", writer_id, num);
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
        Sleep((rand() % 3 + 1) * 1000);
        // Ожидание разрешения на чтение
        start_read();
        if (!sig_flag)
            break;
        printf("Reader %ld -> %d\n", reader_id, num);
        // Завершаем чтение
        stop_read();
    }
    return 0;
}


// Устанавливаем флаг при получении сигнала
int WINAPI signal_handler(DWORD signal) 
{
    printf("SIGNAL");
    if (signal == CTRL_C_EVENT) 
    {
        sig_flag = 0;
        SetEvent(can_read);  // Разрешить заблокированным потокам завершиться
        SetEvent(can_write); // Аналогично для писателей
        printf("Program catch Ctrl-C.\n");
    }
    return 1;
}


int main(void)
{
    setbuf(stdout, NULL);

    // Устанавливаем обработчик сигнала
	if (! SetConsoleCtrlHandler(signal_handler, TRUE)) 
    {
    	perror("Error: set signal handler.\n");
    	exit(EXIT_FAILURE);
    }

    HANDLE threads_read[READ_CNT]; // Массив для дескрипторов потоков читателей
    HANDLE threads_write[WRIT_CNT]; // Массив для дескрипторов потоков писателей

    // Создаем мьютекс для синхронизации доступа к общей переменной
    if ((mutex_num = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        perror("Error: CreateMutex");
        return 1;
    }

    // Создаем события для синхронизации чтения и записи
    can_read = CreateEvent(NULL, TRUE, FALSE, NULL); // со сбросом вручную
    can_write = CreateEvent(NULL, FALSE, FALSE, NULL); // с автосбросом
    if (can_read == NULL || can_write == NULL)
    {
        perror("Error: CreateEvent");
        return 1;
    }

    // ЗАПУСКАЕМ ПРОЦЕССЫ-ПИСАТЕЛИ
    for (int i = 0; i < WRIT_CNT; i++)
    {
        threads_write[i] = CreateThread(NULL, 0, func_writer, (LPVOID)(intptr_t)i, 0, NULL);
        if (threads_write[i] == NULL)
        {           
            DWORD errorCode = GetLastError();
            printf("FError: CreateThread. Error code: %lu\n", errorCode);
            return 1;
        }
    }

    // ЗАПУСКАЕМ ПРОЦЕССЫ-ЧИТАТЕЛИ
    for (int i = 0; i < READ_CNT; i++)
    {
        threads_read[i] = CreateThread(NULL, 0, func_reader, (LPVOID)(intptr_t)i, 0, NULL);
        if (threads_read[i] == NULL)
        {
            DWORD errorCode = GetLastError();
            printf("FError: CreateThread. Error code: %lu\n", errorCode);
            return 1;
        }
    }

    // Ожидаем завершения всех потоков
    WaitForMultipleObjects(READ_CNT, threads_read, TRUE, INFINITE);
    WaitForMultipleObjects(WRIT_CNT, threads_write, TRUE, INFINITE);

    // Закрываем все дескрипторы
    CloseHandle(mutex_num);
    CloseHandle(can_read);
    CloseHandle(can_write);

    return 0;
}