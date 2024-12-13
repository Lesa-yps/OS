/*
 * filename: pc.x
     * function: Define constants, non-standard data types and the calling process in remote calls
 */

struct PROD_CONS
{
    int type;
    char result;
};

program PC_PROG
{
    version PC_VER
    {
        struct PROD_CONS SERVICE(struct PROD_CONS) = 1;
    } = 1; /* Version number = 1 */
} = 0x20000001; /* RPC program number */