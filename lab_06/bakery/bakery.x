/*
 * filename: bakery.x
     * function: Define constants, non-standard data types and the calling process in remote calls
 */

const ADD = 0;
const SUB = 1;
const MUL = 2;
const DIV = 3;

struct REQUEST
{
    int index;
    int number;
    int pid;

    int op;
    float arg1;
    float arg2;
    float result;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        struct REQUEST GET_NUMBER(struct REQUEST) = 1;
        float BAKERY_SERVICE(struct REQUEST) = 2;
    } = 1; /* Version number = 1 */
} = 0x20000001; /* RPC program number */