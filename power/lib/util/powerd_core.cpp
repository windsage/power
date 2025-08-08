//#ifdef __cplusplus
//extern "C" {
//#endif

/*** STANDARD INCLUDES *******************************************************/
#define LOG_NDEBUG 0 // support ALOGV

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

/*** PROJECT INCLUDES ********************************************************/
#define PERFD

#include "ports.h"
#include "mi_types.h"
#include "mi_util.h"
#include "ptimer.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "mtkpower@impl"
#endif

//#define MSG_POST_HANDLE

#include "power_util.h"
#include "powerd_int.h"
#include "powerd_core.h"
#include "mtkpower_hint.h"
#include "mtkpower_types.h"
#include "perfservice.h"
#include "perfservice_types.h"
#include <vector>

#include <vendor/mediatek/hardware/mtkpower/1.1/IMtkPerf.h>
#include <vendor/mediatek/hardware/mtkpower/1.1/IMtkPower.h>

//#define PERFD_DEBUG_LOG

//using namespace vendor::mediatek::hardware::power::V2_0;
using namespace vendor::mediatek::hardware::mtkpower::V1_1;
using std::vector;

/* Global variable */
static void * _gpTimerMng;

static int gMyPid = 0;
static int gMyTid = 0;

static int powerdOnOff = 1;

#define MAX_TIMER_COUNT             256
#define CHECK_USER_SCN_DURATION  300000

enum {
    TIMER_MSG_POWER_HINT_ENABLE_TIMEOUT = 0,
    TIMER_MSG_PERF_LOCK_TIMEOUT         = 1,
    TIMER_MSG_USER_SCN_ENABLE_TIMEOUT   = 2,
    TIMER_MSG_CHECK_USER_SCN_TIMEOUT    = 3,
};

struct tTimer {
    int used;
    int idx;
    int msg;
    int handle;
    void *p_pTimer;
};

typedef struct tPostMsg {
    int msg;
    int hint;
    int hdl;
    int size;
    int pid;
    int tid;
    int duration;
    int rsc[MAX_ARGS_PER_REQUEST];
} tPostMsg;

static int nPerfSupport = 1;
struct tTimer powerdTimer[MAX_TIMER_COUNT]; // temp

static vector<tPostMsg> vPostMsg;

//static int cusHintTblSize = 0;

/*****************
   Function
 *****************/
int reset_timer(int i)
{
    if (i >= 0 && i < MAX_TIMER_COUNT) {
        powerdTimer[i].used = 0;
        powerdTimer[i].msg = -1;
        powerdTimer[i].handle = -1;
        powerdTimer[i].p_pTimer = NULL;
    }
    return 0;
}

int allocate_timer(void)
{
    int i;
    for(i=0; i<MAX_TIMER_COUNT; i++) {
        if(powerdTimer[i].used == 0) {
            powerdTimer[i].used = 1;
            return i;
        }
    }
    return -1;
}

int find_timer(int msg, int handle)
{
    int i;
    for(i=0; i<MAX_TIMER_COUNT; i++) {
        if(powerdTimer[i].used && powerdTimer[i].handle == handle && powerdTimer[i].msg == msg)
            return i;
    }
    return -1;
}

int remove_scn_timer(int msg, int handle)
{
    int idx;

    if((idx = find_timer(msg, handle)) >= 0) {
        ptimer_stop(powerdTimer[idx].p_pTimer);
        ptimer_delete(powerdTimer[idx].p_pTimer);
        reset_timer(idx);
    }
    return 0;
}

int start_scn_timer(int msg, int handle, int timeout)
{
    int idx;
    if((idx = allocate_timer()) >= 0) {
        //ALOGI("[start_scn_timer] idx:%d, handle:%d, timeout:%d", idx, handle, timeout);
        powerdTimer[idx].msg = msg;
        powerdTimer[idx].handle = handle;
        ptimer_create(&(powerdTimer[idx].p_pTimer));
        ptimer_start(_gpTimerMng, powerdTimer[idx].p_pTimer, timeout, &(powerdTimer[idx]));
    }
    return 0;
}

int start_periodic_check_timer(int firstLaunch)
{
    static int idx = 0;

    if(firstLaunch) {
        if((idx = allocate_timer()) >= 0) {
            //ALOGI("[start_scn_timer] idx:%d, handle:%d, timeout:%d", idx, handle, timeout);
            powerdTimer[idx].msg = TIMER_MSG_CHECK_USER_SCN_TIMEOUT;
            powerdTimer[idx].handle = 0;
            ptimer_create(&(powerdTimer[idx].p_pTimer));
        }
        else
            return 0;
    }

    if (idx >= 0 && idx < MAX_TIMER_COUNT)
        ptimer_start(_gpTimerMng, powerdTimer[idx].p_pTimer, CHECK_USER_SCN_DURATION, &(powerdTimer[idx]));
    return 0;
}

int powerd_core_pre_init(void)
{
    gMyPid = (int)getpid();
    gMyTid = (int)gettid();

    return 0;
}

int powerd_core_init(void * pTimerMng)
{
    int i = 0;

    _gpTimerMng = pTimerMng;

    for(i=0; i<MAX_TIMER_COUNT; i++) {
        powerdTimer[i].used = 0;
        powerdTimer[i].idx = i;
        powerdTimer[i].msg = -1;
        powerdTimer[i].handle = -1;
        powerdTimer[i].p_pTimer = NULL;
    }

    /* start periodic timer to check invalid user scenario */
    if (nPerfSupport)
        start_periodic_check_timer(1);

    /* post message */
    vPostMsg.clear();
    return 0;
}

#ifdef MSG_POST_HANDLE
static int add_post_req(int msg, int hdl, int size, int pid, int tid, int duration, int* list)
{
    tPostMsg newMsg;
    int i;

    if (hdl <= 0)
        return 0;

    LOG_D("hdl:%d, size:%d, duration:%d", hdl, size, duration);
    for (i=0; i<size; i+=2)
        LOG_D("rsc:%8x data:%d", list[i], list[i+1]);

    newMsg.msg = msg;
    newMsg.hdl = hdl;
    newMsg.size = size;
    newMsg.pid = pid;
    newMsg.tid = tid;
    newMsg.duration = duration;
    if (size > 0 && size <= MAX_ARGS_PER_REQUEST)
        memcpy(newMsg.rsc, list, sizeof(int)*size);

    vPostMsg.push_back(newMsg);
    return 0;
}
#endif // MSG_POST_HANDLE

int powerd_core_timer_handle(const void * pTimer, void * pData)
{
    //int i = 0;
    struct tTimer *ptTimer = (struct tTimer *)pData;

    if(ptTimer->p_pTimer != pTimer || !powerdOnOff)
        return -1;

    switch(ptTimer->msg) {
#if 0
    case TIMER_MSG_POWER_HINT_ENABLE_TIMEOUT:
        if (nPerfSupport)
            perfBoostDisable(ptTimer->handle);
        ptimer_delete(ptTimer->p_pTimer);
        reset_timer(ptTimer->idx);
        break;
#endif

    case TIMER_MSG_PERF_LOCK_TIMEOUT:
//#ifdef PERFD_DEBUG_LOG
        ALOGI("[powerd_req] TIMER_MSG_PERF_LOCK_TIMEOUT hdl:%d", ptTimer->handle);
//#endif
        if (nPerfSupport)
            perfLockRel(ptTimer->handle);
        ptimer_delete(ptTimer->p_pTimer);
        reset_timer(ptTimer->idx);
        break;

    case TIMER_MSG_CHECK_USER_SCN_TIMEOUT:
        if (nPerfSupport) {
            perfUserScnCheckAll();
            perfDumpAll(0);
        }
        start_periodic_check_timer(0);
        break;
    }

    return 0;
}

//extern "C"
int powerd_req(void * pMsg, void ** pRspMsg)
{
    struct tScnData    *vpScnData = NULL;
    //struct tScnData    *vpRspScn = NULL;
    //struct tHintData   *vpHintData = NULL;
    struct tPerfLockData *vpPerfLockData = NULL, *vpRspPerfLock = NULL;
    struct tCusLockData *vpCusLockData = NULL, *vpRspCusLockData = NULL;
    struct tAppStateData *vpAppState = NULL;
    struct tQueryInfoData  *vpQueryData = NULL, *vpRspQuery = NULL;
    struct tPowerData * vpData = (struct tPowerData *) pMsg;
    struct tPowerData * vpRsp = NULL;
    struct tSysInfoData *vSysInfoData = NULL, *vpRspSysInfo = NULL;
    struct tTimerData *vpTimerData = NULL;
    int hdl = 0;
#ifdef MSG_POST_HANDLE
    int hint_rsc[MAX_ARGS_PER_REQUEST], hint_rsc_size = 0;
#endif
    //struct tHintData        *vpRspHint;
    //struct tAppStateDate    *vpRspAppState;

    if(pMsg == NULL)
        return -1;

    if(!nPerfSupport) {
        if(vpData->msg != POWER_MSG_MTK_HINT) // log reduction
            LOG_E("libpowerhal not supported");
        return -1;
    }

    if((vpRsp = (struct tPowerData *) malloc(sizeof(struct tPowerData))) == NULL) {
        LOG_E("malloc failed");
        return -1;
    }

    if(vpRsp) {
        vpRsp->msg = vpData->msg;
        vpRsp->pBuf = NULL;
    }

    switch(vpData->msg) {

    case POWER_MSG_SCN_DISABLE_ALL:
        if(vpData->pBuf && powerdOnOff) {
            LOG_D("POWER_MSG_SCN_DISABLE_ALL");
            vpScnData = (struct tScnData*)(vpData->pBuf);
            perfUserScnDisableAll();
        }
        break;

    case POWER_MSG_SCN_RESTORE_ALL:
        if(vpData->pBuf && powerdOnOff) {
            LOG_D("POWER_MSG_SCN_RESTORE_ALL");
            vpScnData = (struct tScnData*)(vpData->pBuf);
            perfUserScnRestoreAll();
        }
        break;

    case POWER_MSG_NOTIFY_STATE:
        LOG_V("POWER_MSG_NOTIFY_STATE");
        if(vpData->pBuf && powerdOnOff) {
            vpAppState = (struct tAppStateData*)(vpData->pBuf);
            LOG_V("POWER_MSG_NOTIFY_STATE: %s, %s, %d, %d, %d", vpAppState->pack, vpAppState->activity, vpAppState->state, vpAppState->pid, vpAppState->uid);
            perfNotifyAppState(vpAppState->pack, vpAppState->activity, vpAppState->state, vpAppState->pid, vpAppState->uid);
        }
        break;

    case POWER_MSG_QUERY_INFO:
        LOG_V("POWER_MSG_QUERY_INFO");

        if(vpData->pBuf) {
            if((vpRspQuery = (struct tQueryInfoData*)malloc(sizeof(struct tQueryInfoData))) == NULL)
                break;
            /* initialize vpRspQuery */
            vpRspQuery->cmd = POWER_MSG_QUERY_INFO;
            vpRspQuery->param = -1;
            vpRspQuery->value = -1;

            vpQueryData = (struct tQueryInfoData*)(vpData->pBuf);
            LOG_D("POWER_MSG_QUERY_INFO: cmd:%d, param:%d", vpQueryData->cmd, vpQueryData->param);

            if(vpQueryData->cmd == MTKPOWER_CMD_GET_DEBUG_DUMP_ALL) {
                perfDumpAll(vpQueryData->param);
            } else if(vpQueryData->cmd == MTKPOWER_CMD_GET_POWERHAL_ONOFF) {
                powerdOnOff = vpQueryData->param;
            } else {
               vpRspQuery->value = perfUserGetCapability(vpQueryData->cmd, vpQueryData->param);
            }
            vpRsp->pBuf = (void*)vpRspQuery;
        }
        break;

    case POWER_MSG_SET_SYSINFO:
        if (vpData->pBuf && powerdOnOff) {
            if((vpRspSysInfo = (struct tSysInfoData*)malloc(sizeof(struct tSysInfoData))) == NULL)
                break;
            memset(vpRspSysInfo, 0, sizeof(struct tSysInfoData));
            vSysInfoData = (struct tSysInfoData*)(vpData->pBuf);
            //LOG_D("POWER_MSG_SET_SYSINFO, type:%d, str:%s", vSysInfoData->type, vSysInfoData->data);
            vpRspSysInfo->ret = perfSetSysInfo(vSysInfoData->type, vSysInfoData->data);
            vpRsp->pBuf = (void*)vpRspSysInfo;
        }
        break;

    case POWER_MSG_PERF_LOCK_ACQ:
        if(vpData->pBuf && powerdOnOff) {
            if((vpRspPerfLock = (struct tPerfLockData*)malloc(sizeof(struct tPerfLockData))) == NULL)
                break;
            memset(vpRspPerfLock, 0, sizeof(struct tPerfLockData));
            vpPerfLockData = (struct tPerfLockData*)(vpData->pBuf);

            LOG_D("POWER_MSG_PERF_LOCK_ACQ: hdl:%d, duration:%d", vpPerfLockData->hdl, vpPerfLockData->duration);

            remove_scn_timer(TIMER_MSG_PERF_LOCK_TIMEOUT, vpPerfLockData->hdl);


#ifdef MSG_POST_HANDLE
            hdl = perfLockCheckHdl(vpPerfLockData->hdl, -1, vpPerfLockData->size,
                vpPerfLockData->pid, vpPerfLockData->reserved, vpPerfLockData->duration);
#else
            hdl = perfLockAcq(vpPerfLockData->rscList, vpPerfLockData->hdl,
                vpPerfLockData->size, vpPerfLockData->pid, vpPerfLockData->reserved, vpPerfLockData->duration, vpPerfLockData->hint);
#endif
            LOG_D("POWER_MSG_PERF_LOCK_ACQ: return hdl:%d", hdl);
            vpRspPerfLock->hdl = hdl;
            vpRsp->pBuf = (void*)vpRspPerfLock;

            if(vpPerfLockData->duration != 0 && hdl > 0) { // not MTK_HINT_ALWAYS_ENABLE
                start_scn_timer(TIMER_MSG_PERF_LOCK_TIMEOUT, hdl, vpPerfLockData->duration);
            }
#ifdef MSG_POST_HANDLE
            add_post_req(POWER_MSG_PERF_LOCK_ACQ, hdl, vpPerfLockData->size, vpPerfLockData->pid,
                vpPerfLockData->reserved, vpPerfLockData->duration, vpPerfLockData->rscList);
#endif
        }
        break;

    case POWER_MSG_CUS_LOCK_HINT:
        if(vpData->pBuf && powerdOnOff) {
            if((vpRspCusLockData = (struct tCusLockData*)malloc(sizeof(struct tCusLockData))) == NULL)
                break;
            memset(vpRspCusLockData, 0, sizeof(struct tCusLockData));
            vpCusLockData = (struct tCusLockData*)(vpData->pBuf);
            if(vpCusLockData->hint >= MTKPOWER_HINT_NUM) {
                LOG_E("unsupport cus lock hint:%d", vpCusLockData->hint);
                free(vpRspCusLockData);
                break;
            }

#ifdef MSG_POST_HANDLE
            perfGetHintRsc(vpCusLockData->hint, &hint_rsc_size, hint_rsc);
            hdl = perfLockCheckHdl(0, vpCusLockData->hint, hint_rsc_size, vpCusLockData->pid, 0, vpCusLockData->duration);
#else
            hdl = perfCusLockHint(vpCusLockData->hint, vpCusLockData->duration, vpCusLockData->pid);
#endif
            vpRspCusLockData->hdl = hdl;
            vpRsp->pBuf = (void*)vpRspCusLockData;

            if(vpCusLockData->duration != 0 && hdl > 0) {
                start_scn_timer(TIMER_MSG_PERF_LOCK_TIMEOUT, hdl, vpCusLockData->duration);
            }

#ifdef MSG_POST_HANDLE
            add_post_req(POWER_MSG_PERF_LOCK_ACQ, hdl, hint_rsc_size, vpCusLockData->pid,
                0, vpCusLockData->duration, hint_rsc);
#endif
        }
        break;

    case POWER_MSG_PERF_LOCK_REL:
        if(vpData->pBuf && powerdOnOff) {
            vpPerfLockData = (struct tPerfLockData*)(vpData->pBuf);

            LOG_D("POWER_MSG_PERF_LOCK_REL: hdl:%d", vpPerfLockData->hdl);

            remove_scn_timer(TIMER_MSG_PERF_LOCK_TIMEOUT, vpPerfLockData->hdl);

            if(vpPerfLockData->hdl) {
                perfLockRel(vpPerfLockData->hdl);
            }
        }
        break;

    case POWER_MSG_TIMER_REQ:
        if(vpData->pBuf) {
            vpTimerData = (struct tTimerData*)(vpData->pBuf);
            LOG_D("POWER_MSG_TIMER_REQ: type:%d, data:%d, duration:%d", vpTimerData->type, vpTimerData->type, vpTimerData->duration);

            if (vpTimerData->type == TIMER_PERF_LOCK_DURATION) {
                remove_scn_timer(TIMER_MSG_PERF_LOCK_TIMEOUT, vpTimerData->data);

                if (vpTimerData->duration > 0) {
                    start_scn_timer(TIMER_MSG_PERF_LOCK_TIMEOUT, vpTimerData->data, vpTimerData->duration);
                }
            } else if(vpTimerData->type == TIMER_HINT_DURATION) {
                LOG_D("TIMER_HINT_DURATION: 1, hint:%d, duration:%d", vpTimerData->data, vpTimerData->duration);
                remove_scn_timer(TIMER_MSG_POWER_HINT_ENABLE_TIMEOUT, vpTimerData->data);
                LOG_D("TIMER_HINT_DURATION: 2");

                if (vpTimerData->duration > 0) {
                    start_scn_timer(TIMER_MSG_POWER_HINT_ENABLE_TIMEOUT, vpTimerData->data, vpTimerData->duration);
                    LOG_D("TIMER_HINT_DURATION: 3");
                }
                LOG_D("TIMER_HINT_DURATION: 4");
            }

        }
        break;

    default:
        LOG_E("unknown message");
        break;
    }

    *pRspMsg = (void *) vpRsp;

    return 0;
}

int powerd_post_req(void)
{
#ifdef MSG_POST_HANDLE
    int i, hdl;

    for (i=0; i<vPostMsg.size(); i++) {

        switch(vPostMsg[i].msg) {

        case POWER_MSG_PERF_LOCK_ACQ:
            LOG_D("POWER_MSG_PERF_LOCK_ACQ");
#ifdef PERFD_DEBUG_LOG
            int j;
            LOG_D("hdl:%d, size:%d, pid:%d, tid:%d, dur:%d", vPostMsg[i].hdl, vPostMsg[i].size,
                vPostMsg[i].pid, vPostMsg[i].tid, vPostMsg[i].duration);
            for (j=0; j<vPostMsg[i].size; j+=2)
                LOG_D("rsc:%8x data:%d", vPostMsg[i].rsc[j], vPostMsg[i].rsc[j+1]);
#endif
            hdl = perfLockAcq(vPostMsg[i].rsc, vPostMsg[i].hdl,
                vPostMsg[i].size, vPostMsg[i].pid, vPostMsg[i].tid, vPostMsg[i].duration, -1);
            break;

        case POWER_MSG_CUS_LOCK_HINT:
            LOG_D("POWER_MSG_CUS_LOCK_HINT");
            break;

        default:
            break;
        }
    }

    vPostMsg.clear();
#endif
    return 0;
}


//#ifdef __cplusplus
//}
//#endif


