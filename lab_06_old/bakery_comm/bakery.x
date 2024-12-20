/*
 * filename: bakery.x
 * 
 * Назначение: 
 * Этот файл определяет интерфейс для удаленных вызовов процедур (RPC).
 * Включает описание констант, типов данных и процедур, 
 * которые могут быть вызваны удаленно.
 */

/* Константы, обозначающие типы операций */
const ADD = 0; /* Операция сложения */
const SUB = 1; /* Операция вычитания */
const MUL = 2; /* Операция умножения */
const DIV = 3; /* Операция деления */

/* 
 * Структура REQUEST: используется для передачи данных между клиентом и сервером.
 * Включает параметры запроса и поле для хранения результата.
 */
struct REQUEST
{
    int index;     /* Уникальный индекс запроса */
    int number;    /* Идентификатор клиента или номер в очереди */
    int pid;       /* Идентификатор процесса клиента */

    int op;        /* Тип операции: ADD, SUB, MUL или DIV */
    float arg1;    /* Первый аргумент операции */
    float arg2;    /* Второй аргумент операции */
    float result;  /* Результат вычисления, возвращаемый сервером */
};

/* 
 * Программа удаленных вызовов процедур (RPC) с уникальным идентификатором.
 * Содержит одну версию с двумя удаленными процедурами.
 */
program BAKERY_PROG
{
    /* Версия программы с номером 1 */
    version BAKERY_VER
    {
        /*
         * Удаленная процедура GET_NUMBER.
         * Принимает структуру REQUEST, возвращает обработанную структуру REQUEST.
         */
        struct REQUEST GET_NUMBER(struct REQUEST) = 1;

        /*
         * Удаленная процедура BAKERY_SERVICE.
         * Принимает структуру REQUEST, возвращает результат вычисления.
         */
        float BAKERY_SERVICE(struct REQUEST) = 1;
    } = 1; /* Номер версии */
} = 0x20000001; /* Уникальный номер программы */
