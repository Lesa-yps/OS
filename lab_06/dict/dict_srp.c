#include<rpc/rpc.h>
#include<string.h>
#include "dict.h"
char dict[DICTSIZ][MAXWORD+1];
static char snd[50];
static int lns;
int nwords=0;
int 
initw()
{
	nwords=0;
	return 1;
}
int 
insertw(char *word)
{
	strcpy(dict[nwords],word);
	nwords++;
	return nwords;
}
int 
deletew(char *word)
{
	int i;
	for(i=0;i<nwords;i++)
	if(strcmp(word,dict[i])==0)
	 {
		nwords--;
		strcpy(dict[i],dict[nwords]);
		return 1;
	 }
	return 0;
}
int 
lookupw(char *word)
{
	int i;
        for(i=0;i < nwords;i++)
        if(strcmp(word,dict[i])==0)
             return 1;
	return 0;
}
rdictprog_1_freeresult(SVCXPRT *transp,xdrproc_t xdr_result,
 caddr_t result)
{
xdr_free(xdr_result,result);
return(1);
}