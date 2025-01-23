/*
 * filename: pc.x
     * function: Define constants, non-standard data types and the calling process in remote calls
 */

program PC_PROG
{
    version PC_VER
    {
        char SERVICE(int type_proc) = 1;
    } = 1; /* Version number = 1 */
} = 0x20000001; /* RPC program number */