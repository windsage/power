
#ifndef __PERFSERVD_CORE_H__
#define __PERFSERVD_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif


/*** STANDARD INCLUDES *******************************************************/


/*** PROJECT INCLUDES ********************************************************/


/*** MACROS ******************************************************************/
#define PS_CLUSTER_MAX 8


/*** GLOBAL TYPES DEFINITIONS ************************************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/
int powerd_core_init(void * pTimerMng);
int powerd_core_timer_handle(const void * pTimer, void * pData);
int powerd_core_init(void * pTimerMng);

//long powerd_lock_aquire(unsigned long handle, int duration, struct tPowerData * pScnData);
//long powerd_lock_rel(unsigned long handle);

int powerd_req(void * pMsg, void ** pRspMsg);
int powerd_post_req(void);

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __PERFSERVD_CORE_H__ */

