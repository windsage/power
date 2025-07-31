
#ifndef __PERFSERVD_CMD_H__
#define __PERFSERVD_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif


/*** STANDARD INCLUDES *******************************************************/


/*** PROJECT INCLUDES ********************************************************/


/*** MACROS ******************************************************************/
#define PS_SCN_TYPE_BASE                0x0000
#define PS_SCN_TYPE_MEMORY              (PS_SCN_TYPE_BASE + 1)
#define PS_SCN_TYPE_PERF_LOCK           (PS_SCN_TYPE_BASE + 2)
#define PS_SCN_TYPE_LOCK_REL            (PS_SCN_TYPE_BASE + 3)
#define PS_SCN_TYPE_PERF_CUS_LOCK_HINT  (PS_SCN_TYPE_BASE + 4)
#define PS_SCN_TYPE_TIMER_REQ           (PS_SCN_TYPE_BASE + 5)


/*** GLOBAL TYPES DEFINITIONS ************************************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/
int powerd_cmd_create(void ** ppcmd);
int powerd_cmd_destory(void * pcmd);


#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __PERFSERVD_CMD_H__ */

