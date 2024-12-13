/*
*    dict_sif.c
*/
#include<rpc/rpc.h>
#define RPC_SVC
#include "dict.h"

int initw(void),insertw(char *),deletew(char *),lookupw(char *);
bool_t
insertw_1_svc(char **w,int *ptr_retcode,struct svc_req *rqstp)
{
	*ptr_retcode=insertw(*(char **)w);
          return(TRUE);
}
bool_t
initw_1_svc(void *w,int *ptr_retcode,struct svc_req *rqstp)
{
        *ptr_retcode=initw();
          return(TRUE);
}
bool_t
deletew_1_svc(char **w,int *ptr_retcode,struct svc_req *rqstp)
{
        *ptr_retcode=deletew(*(char **)w);
          return(TRUE);
}
bool_t
lookupw_1_svc(char **w,int *ptr_retcode,struct svc_req *rqstp)
{
        *ptr_retcode=lookupw(*(char **)w);
          return(TRUE);
}
