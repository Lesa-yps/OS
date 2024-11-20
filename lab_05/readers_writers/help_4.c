#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

// Количество читателей и писателей
#define READERS_COUNT 5
#define WRITERS_COUNT 3

// Количество итераций для каждого читателя и писателя
#define READ_ITERS 7
#define WRITE_ITERS 8

// Время ожидания для операций чтения и записи (в миллисекундах)
#define READ_TIMEOUT 300
#define WRITE_TIMEOUT 300

// Разница в времени для случайного времени простоя
#define DIFF 4000

// Объекты синхронизации
HANDLE mutex;        // Мьютекс для защиты общей переменной (val)
HANDLE can_read;     // Событие для разрешения чтения
HANDLE can_write;    // Событие для разрешения записи

// Переменные для отслеживания состояния
LONG waiting_writers = 0;   // Количество ожидающих писателей
LONG waiting_readers = 0;   // Количество ожидающих читателей
LONG active_readers = 0;    // Количество активных читателей
bool active_writer = false; // Флаг для отслеживания наличия активного писателя

int val = 0; // Общая переменная, с которой работают читатели и писатели

// Функция для начала чтения
void start_read()
{
    // Увеличиваем количество ожидающих читателей
    InterlockedIncrement(&waiting_readers);

    // Если есть активный писатель или ожидающие писатели, ждем разрешения на чтение
    if (active_writer || (WaitForSingleObject(can_write, 0) == WAIT_OBJECT_0 && waiting_writers))
    {
        WaitForSingleObject(can_read, INFINITE);
    }
    
    // Блокируем доступ к общей переменной
    WaitForSingleObject(mutex, INFINITE);

    // Уменьшаем количество ожидающих читателей и увеличиваем количество активных читателей
    InterlockedDecrement(&waiting_readers);
    InterlockedIncrement(&active_readers);

    // Разрешаем чтение
    SetEvent(can_read);
    ReleaseMutex(mutex); // Освобождаем мьютекс
}

// Функция для завершения чтения
void stop_read()
{
    // Уменьшаем количество активных читателей
    InterlockedDecrement(&active_readers);

    // Если нет активных читателей, разрешаем запись
    if (active_readers == 0)
    {
        ResetEvent(can_read);  // Отключаем событие can_read
        SetEvent(can_write);   // Разрешаем запись
    }
}

// Функция для начала записи
void start_write(void)
{
    // Увеличиваем количество ожидающих писателей
    InterlockedIncrement(&waiting_writers);

    // Если есть активный читатель или писатель, ждем разрешения на запись
    if (active_writer || active_readers > 0)
    {
        WaitForSingleObject(can_write, INFINITE);
    }

    // Уменьшаем количество ожидающих писателей
    InterlockedDecrement(&waiting_writers);

    // Устанавливаем флаг активного писателя
    active_writer = true;
}

// Функция для завершения записи
void stop_write(void)
{
    // Снимаем флаг активного писателя
    active_writer = false;

    // Если есть ожидающие читатели, разрешаем им читать
    if (waiting_readers)
    {
        SetEvent(can_read);
    }
    else
    {
        SetEvent(can_write); // Иначе, разрешаем запись
    }
}

// Функция для работы читателя
DWORD WINAPI rr_run(CONST LPVOID lpParams)
{
    int r_id = (int)lpParams;    // Идентификатор читателя
    srand(time(NULL) + r_id);    // Инициализация генератора случайных чисел

    int stime; // Время простоя

    for (size_t i = 0; i < READ_ITERS; i++)
    {
        stime = READ_TIMEOUT + rand() % DIFF; // Случайное время простоя
        Sleep(stime); // Чтение занимает некоторое время
        start_read(); // Ожидание разрешения на чтение
        printf("?Reader #%d read: %3d // Idle time: %dms\n", r_id, val, stime); // Чтение значения
        stop_read(); // Завершаем чтение
    }

    return 0;
}

// Функция для работы писателя
DWORD WINAPI wr_run(CONST LPVOID lpParams)
{
    int w_id = (int)lpParams;    // Идентификатор писателя
    srand(time(NULL) + w_id + READERS_COUNT); // Инициализация генератора случайных чисел для писателя

    int stime; // Время простоя

    for (size_t i = 0; i < WRITE_ITERS; ++i)
    {
        stime = WRITE_TIMEOUT + rand() % DIFF; // Случайное время простоя
        Sleep(stime); // Запись занимает некоторое время
        start_write(); // Ожидание разрешения на запись
        ++val; // Запись нового значения
        printf("!Writer #%d wrote: %3d // Idle time: %dms\n", w_id, val, stime); // Писатель записал значение
        stop_write(); // Завершаем запись
    }
    return 0;
}

int main()
{
    setbuf(stdout, NULL); // Отключаем буферизацию вывода

    HANDLE readers_threads[READERS_COUNT]; // Массив для потоков читателей
    HANDLE writers_threads[WRITERS_COUNT]; // Массив для потоков писателей

    // Создаем мьютекс для синхронизации доступа к общей переменной
    if ((mutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        perror("Failed call of CreateMutex");
        return -1;
    }

    // Создаем события для синхронизации чтения и записи
    can_read = CreateEvent(NULL, FALSE, FALSE, NULL);
    can_write = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (can_read == NULL || can_write == NULL)
    {
        perror("Failed call of CreateEvent");
        return -1;
    }

    // Создаем потоки для читателей
    for (size_t i = 0; i < READERS_COUNT; ++i)
    {
        readers_threads[i] = CreateThread(NULL, 0, rr_run, (LPVOID)i, 0, NULL);
        if (readers_threads[i] == NULL)
        {
            perror("Failed call of CreateThread");
            return -1;
        }
    }

    // Создаем потоки для писателей
    for (size_t i = 0; i < WRITERS_COUNT; ++i)
    {
        writers_threads[i] = CreateThread(NULL, 0, wr_run, (LPVOID)i, 0, NULL);
        if (writers_threads[i] == NULL)
        {
            perror("Failed call of CreateThread");
            return -1;
        }
    }

    // Ожидаем завершения всех потоков
    WaitForMultipleObjects(READERS_COUNT, readers_threads, TRUE, INFINITE);
    WaitForMultipleObjects(WRITERS_COUNT, writers_threads, TRUE, INFINITE);

    // Закрываем все дескрипторы
    CloseHandle(mutex);
    CloseHandle(can_read);
    CloseHandle(can_write);

    return 0;
}
