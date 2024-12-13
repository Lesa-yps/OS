/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _PC_H_RPCGEN
#define _PC_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


#define PC_PROG 0x20000001
#define PC_VER 1

#if defined(__STDC__) || defined(__cplusplus)
#define PRODUCE 1
extern  void * produce_1(void *, CLIENT *);
extern  void * produce_1_svc(void *, struct svc_req *);
#define CONSUME 2
extern  char * consume_1(void *, CLIENT *);
extern  char * consume_1_svc(void *, struct svc_req *);
extern int pc_prog_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define PRODUCE 1
extern  void * produce_1();
extern  void * produce_1_svc();
#define CONSUME 2
extern  char * consume_1();
extern  char * consume_1_svc();
extern int pc_prog_1_freeresult ();
#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_PC_H_RPCGEN */
