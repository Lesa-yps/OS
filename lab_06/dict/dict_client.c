/*   dict_client.c измененный в соответствии с логикой приложения
 *
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include <ctype.h>
#include "dict.h"
#define MAXWORD 50

char buf[80];
void
rdictprog_1(char *host)
{
	CLIENT *clnt;
	enum clnt_stat retval_1;
	int result_1;
	char *initw_1_arg="0";
	enum clnt_stat retval_2;
	int result_2;
	char * insertw_1_arg;
	enum clnt_stat retval_3;
	int result_3;
	char * deletew_1_arg;
	enum clnt_stat retval_4;
	int result_4;
	char * lookupw_1_arg;
int ch;
char cmd;
char word[MAXWORD+1];
int wrdlen;

#ifndef	DEBUG
	clnt = clnt_create (host, RDICTPROG, RDICTVERS, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */
while(1)
{
wrdlen=nextin(&cmd,word);
if(wrdlen < 0)
 exit(0);
word[wrdlen]='\0';
switch(buf[0])
{
 case 'I':
	retval_1 = initw_1((void*)&initw_1_arg, &result_1, clnt);
	if (retval_1 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
       if (result_1 == 1)
        printf("Dictionary was initialized \n"); 
       else
        printf("Dictionary failed to initialize \n");
       break;
 case 'i':
        insertw_1_arg=word;
	retval_2 = insertw_1(&insertw_1_arg, &result_2, clnt);
	if (retval_2 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
    if (result_2 >0 )
      printf("Insert was done\n");
    else
      printf("Insert failed\n");
        break;
 case 'd':
        deletew_1_arg=word;
	retval_3 = deletew_1(&deletew_1_arg, &result_3, clnt);
	if (retval_3 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
     if (result_3 == 1 )
        printf("Delete was done\n");
     else
       printf("Delete failed\n");
        break;
case 'l':
        lookupw_1_arg=word;
	retval_4 = lookupw_1(&lookupw_1_arg, &result_4, clnt);
	if (retval_4 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
        if (result_4 == 1)
          printf("Word '%s' was found\n",word);
        else
          printf("Word '%s' was not found\n",word);
        break;
case 'q':
        printf("Programm quits \n");
        exit(0);
default:
       printf("Command invalid\n");
       break;
 }
}
#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}
int
nextin(char *cmd,char *word)
{
        int i,ch;
        printf("\n");
        printf("***** Make a choice ******\n");
        printf("1. I(initialize dictionary)\n");
        printf("2. i(inserting word) \n");
        printf("3. l(looking for word)\n");
        printf("4. d(deleting word)\n");
        printf("5. q(quit)\n");
        printf("***************************\n");
        printf("Command prompt =>\t");
        ch=getc(stdin);
        while(isspace(ch))
                ch=getc(stdin);
        if(ch==EOF)
                return -1;
        *cmd=(char)ch;
        sprintf(buf,"%s",cmd);
        if(buf[0] == 'q' || buf[0] == 'I')
                 return 0;
         printf("*****************\n");
         printf("Analysing Command\n");
         printf("*****************\n");
         if(buf[0]=='i' || buf[0]=='l'|| buf[0]=='d')
          {
            printf("Input word  =>\t");
          }
          else
          {
              return 0;
          }
         ch=getc(stdin);
        while(isspace(ch))
               ch=getc(stdin);
        if(ch==EOF)
                return -1;
        if(ch=='\n')
                return 0;
        i=0;
        while(!isspace(ch))
        {
                if(++i>MAXWORD)
                {
                        printf("Error word to long.\n");
                        exit(1);
                }
                *word++=ch;
                ch=getc(stdin);
        }
        return i;
}
int
main (int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	rdictprog_1 (host);
exit (0);
}