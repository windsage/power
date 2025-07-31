/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "mtkpower@impl"
#define ATRACE_TAG ATRACE_TAG_PERF

#include <log/log.h>
#include <cutils/trace.h>
#include <string.h>
#include <hardware/hardware.h>
#include <hardware/power.h>
#include <time.h>
#include <pthread.h>
#include <vector>

#include "perfservice.h"
//#include "power_cus_types.h"
#include "power_util.h"
#include "perfservice_scn.h"
#include "mtkpower_types.h"
#include "mtkpower_hint.h"
#include "power_touch_handler.h"
#include "power_error_handler.h"
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPower.h>
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPowerCallback.h>

using namespace vendor::mediatek::hardware::mtkpower::V1_2;
using ::vendor::mediatek::hardware::mtkpower::V1_2::IMtkPowerCallback;
using ::android::sp;

extern int perfSetScnUpdateCallback(int scenario_set, const sp<IMtkPowerCallback> &callback);


#define __INTRA_PROCESS_ONLY__

extern "C" int libpowerhal_NotifyAppState(const char *packName, const char *actName, int state, int pid, int uid);
extern "C" int libpowerhal_CusLockHint(int hint, int duration, int pid);
extern "C" int libpowerhal_CusLockHint_data(int hint, int duration, int pid, int* extendedList, int extendedListSize);
extern "C" int libpowerhal_LockRel(int hdl);

enum {
    WORK_MSG_LOCK_ACQ,
    WORK_MSG_LOCK_REL,
    WORK_MSG_EXIT,
};

typedef struct tWorkMsg {
    int msg;
    int hint;
    int hdl;
    int size;
    int pid;
    int tid;
    int duration;
    int *rsc;
} tWorkMsg;

static vector<tWorkMsg> vWorkMsg;
static Mutex sMutex;
static pthread_mutex_t g_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
static int gWorkerExist = 0;

static int add_work_req(int msg, int hdl, int size, int pid, int tid, int duration, int hint, int* list)
{
    tWorkMsg newMsg;
    LOG_D("msg:%d, hdl:%d", msg, hdl);

    if(!gWorkerExist) {
        LOG_E("there is no worker");
        return 0;
    }

    LOG_D("msg:%d, hdl:%d, size:%d, duration:%d", msg, hdl, size, duration);
#ifdef PERFD_DEBUG_LOG
    int i;
    for (i=0; i<size; i+=2)
        LOG_D("rsc:%8x data:%d", list[i], list[i+1]);
#endif

    newMsg.msg = msg;
    newMsg.hdl = hdl;
    newMsg.size = size;
    newMsg.pid = pid;
    newMsg.tid = tid;
    newMsg.duration = duration;
    newMsg.hint = hint;
    if (size > 0 && size <= MAX_ARGS_PER_REQUEST && list != NULL) {
        if ((newMsg.rsc = (int*)malloc(sizeof(int)*size)) == NULL) {
            ALOGE("Cannot allocate memory for newMsg.rsc");
            return 0;
        }
        memcpy(newMsg.rsc, list, sizeof(int)*size);
    }

    vWorkMsg.push_back(newMsg);
    return 0;
}

static int check_work_req(int msg, int hdl)
{
    int i;
    int wait = true;

    if(!gWorkerExist) {
        LOG_E("there is no worker");
        return 0;
    }

    do {
        wait = false;
        pthread_mutex_lock(&g_cond_mutex);
        LOG_D("work size:%lu", (unsigned long)vWorkMsg.size());
        for (i=0; i<vWorkMsg.size(); i++) {
            if(vWorkMsg[i].msg == msg && vWorkMsg[i].hdl == hdl) {
                LOG_I("i:%d, msg:%d hdl:%d, wait", i, msg, hdl);
                wait = true;
                break;
            }
        }
        pthread_mutex_unlock(&g_cond_mutex);

        if(wait) {
            usleep(2000); // 2ms
        }
    } while (wait);
    return 0;
}

static int libpowerhal_LockAcq_internal(int *list, int hdl, int size, int pid, int tid, int duration, int hint)
{
    LOG_D("hdl:%d, size:%d, duration:%d", hdl, size, duration);
    struct tPowerData vPowerData;
    struct tPerfLockData vPerfLockData;

    vPerfLockData.hdl = hdl;
    vPerfLockData.duration = duration;
    vPerfLockData.size = size;
    vPerfLockData.reserved = tid;
    vPerfLockData.pid = pid;
    vPerfLockData.hint = hint;
    vPowerData.msg  = POWER_MSG_PERF_LOCK_ACQ;

    if ((vPerfLockData.rscList = (int*)malloc(sizeof(int)*size)) == NULL) {
        ALOGE("Cannot allocate memory for vPerfLockData.rscList");
        return 0;
    }
    memcpy(vPerfLockData.rscList, list, sizeof(int)*size);
#ifdef PERFD_DEBUG_LOG
    int i;
    for (i=0; i<vPerfLockData.size; i+=2)
        LOG_D("rsc:0x%08x, value:%d", vPerfLockData.rscList[i], vPerfLockData.rscList[i+1]);
#endif

#ifdef __INTRA_PROCESS_ONLY__
    struct tPowerData *vpRspData = NULL;
    vPowerData.pBuf = (void*)&vPerfLockData;

    power_msg(&vPowerData, (void **) &vpRspData);

    if (vpRspData) {
        if(vpRspData->pBuf) {
            vPerfLockData.hdl = ((tPerfLockData*)(vpRspData->pBuf))->hdl;
            free(vpRspData->pBuf);
        }
        free(vpRspData);
    }
#else
    struct tPowerData vRspData;
    vPowerData.pBuf = (void*)&vPerfLockData;
    vRspData.pBuf = (void*)&vPerfLockData;

    power_msg_perf_lock_acq(&vPowerData, (void **) &vRspData);
#endif
    free(vPerfLockData.rscList);
    return vPerfLockData.hdl;
}

static int libpowerhal_LockRel_internal(int hdl)
{
    LOG_D("hdl:%d", hdl);
#ifdef __INTRA_PROCESS_ONLY__
    struct tPowerData vPowerData;
    struct tPowerData *vpRspData = NULL;
    struct tPerfLockData vPerfLockData;

    vPerfLockData.hdl = hdl;
    vPowerData.msg  = POWER_MSG_PERF_LOCK_REL;
    vPowerData.pBuf = (void*)&vPerfLockData;

    power_msg(&vPowerData, (void **) &vpRspData);

    if (vpRspData) {
        if(vpRspData->pBuf) {
            vPerfLockData.hdl = ((tPerfLockData*)(vpRspData->pBuf))->hdl;
            free(vpRspData->pBuf);
        }
        free(vpRspData);
    }
#else
    struct tPowerData vPowerData;
    struct tPowerData vRspData;
    struct tLockHdlData vLockHdlData;

    vLockHdlData.hdl = hdl;
    vPowerData.msg  = POWER_MSG_PERF_LOCK_REL;
    vPowerData.pBuf = (void*)&vLockHdlData;
    vRspData.pBuf = (void*)&vLockHdlData;

    power_msg_perf_lock_rel(&vPowerData, (void *) &vRspData);
#endif
    return 0;
}


/*************
 * Worker
 *************/
static void* mtkPowerMsgWorker(void *data)
{
    int i, msg_exit = 0;
    bool need_wait;
    LOG_V("%p", data);

    while(1) {
        pthread_mutex_lock(&g_cond_mutex);
        need_wait = (vWorkMsg.size() == 0) ? true : false;
        if (need_wait) {
            pthread_cond_wait(&g_cond, &g_cond_mutex);
        }

        for (i=0; i<vWorkMsg.size(); i++) {
            LOG_D("msg:%d, hdl:%d, size:%d, pid:%d, tid:%d, dur:%d", vWorkMsg[i].msg, vWorkMsg[i].hdl, vWorkMsg[i].size,
                vWorkMsg[i].pid, vWorkMsg[i].tid, vWorkMsg[i].duration);

            switch(vWorkMsg[i].msg) {

            case WORK_MSG_LOCK_ACQ:
        #ifdef PERFD_DEBUG_LOG
                for (j=0; j<vWorkMsg[i].size; j+=2)
                    LOG_I("rsc:%8x data:%d", vWorkMsg[i].rsc[j], vWorkMsg[i].rsc[j+1]);
        #endif
                libpowerhal_LockAcq_internal(vWorkMsg[i].rsc, vWorkMsg[i].hdl, vWorkMsg[i].size, vWorkMsg[i].pid,
                    vWorkMsg[i].tid, vWorkMsg[i].duration, vWorkMsg[i].hint);
                free(vWorkMsg[i].rsc);
                break;

            case WORK_MSG_LOCK_REL:
                libpowerhal_LockRel_internal(vWorkMsg[i].hdl);
                break;

            /* special case: thread exit */
            case WORK_MSG_EXIT:
                LOG_E("WORK_MSG_EXIT");
                msg_exit = 1;
                break;

            default:
                LOG_E("unknown msg:%d", vWorkMsg[i].msg);
                break;
            }
        }

        vWorkMsg.clear();
        pthread_mutex_unlock(&g_cond_mutex);

        /* special case: thread exit */
        if (msg_exit)
            break;
    }
    return NULL;
}

static int createPowerMsgWorker()
{
    pthread_t workerThread;
    pthread_attr_t attr;

    /* handler */
    pthread_attr_init(&attr);
    pthread_create(&workerThread, &attr, mtkPowerMsgWorker, NULL);
    pthread_setname_np(workerThread, "mtkPowerWorker");

    return 0;
}

extern "C"
int libpowerhal_Init(int needWorker)
{
    Mutex::Autolock lock(sMutex);
    static int api_init = 0;

    if(api_init)
        return 0;
    api_init = 1;

    vWorkMsg.clear();
    if(needWorker) {
        createPowerMsgWorker();
        gWorkerExist = 1;
    }

    create_error_handler();
    create_touch_stop();

    return perfLibpowerInit();
}

extern "C"
int libpowerhal_UserScnDisableAll(void)
{
    struct tPowerData vPowerData;
    struct tPowerData *vpRspData = NULL;
    struct tScnData vScnData;

    Mutex::Autolock lock(sMutex);

    LOG_D("");
    vPowerData.msg  = POWER_MSG_SCN_DISABLE_ALL;
    vPowerData.pBuf = (void*)&vScnData;

    power_msg(&vPowerData, (void **) &vpRspData);

    if (vpRspData) {
        if(vpRspData->pBuf)
            free(vpRspData->pBuf);
        free(vpRspData);
    }
    return 0;
}

extern "C"
int libpowerhal_UserScnRestoreAll(void)
{
    struct tPowerData vPowerData;
    struct tPowerData *vpRspData = NULL;
    struct tScnData vScnData;

    Mutex::Autolock lock(sMutex);

    vPowerData.msg  = POWER_MSG_SCN_RESTORE_ALL;
    vPowerData.pBuf = (void*)&vScnData;

    LOG_D("");
    power_msg(&vPowerData, (void **) &vpRspData);

    if (vpRspData) {
        if(vpRspData->pBuf)
            free(vpRspData->pBuf);
        free(vpRspData);
    }
    return 0;
}

extern "C"
int libpowerhal_LockAcq(int *list, int hdl, int size, int pid, int tid, int duration)
{
    if (!queryAPIstatus(STATUS_PERFLOCKACQ_ENABLED)) {
        LOG_D("libpowerhal_LockAcq is disabled");
        return 0;
    }

    LOG_D("hdl:%d, size:%d, duration:%d", hdl, size, duration);

    Mutex::Autolock lock(sMutex);

    if (size % 2 != 0 || size > MAX_ARGS_PER_REQUEST || size == 0) {
        LOG_E("wrong data size:%d", size);
        return 0;
    }

#ifdef PERFD_DEBUG_LOG
    int i;
    for (i=0; i<size; i+=2)
        LOG_D("rsc:0x%08x, value:%d", list[i], list[i+1]);
#endif

    if (gWorkerExist) {
        if (hdl > 0)
            check_work_req(WORK_MSG_LOCK_REL, hdl);
        hdl = perfLockCheckHdl(hdl, -1, size, pid, tid, duration);

        pthread_mutex_lock(&g_cond_mutex);
        add_work_req(WORK_MSG_LOCK_ACQ, hdl, size, pid, tid, duration, -1, list); // hint -1
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_cond_mutex);
    } else {
        hdl = libpowerhal_LockAcq_internal(list, hdl, size, pid, tid, duration, -1); // hint -1
    }

    return hdl;
}

extern "C"
int libpowerhal_CusLockHint(int hint, int duration, int pid)
{
    if (!queryAPIstatus(STATUS_CUSLOCKHINT_ENABLED)) {
        LOG_D("libpowerhal_CusLockHint is disabled");
        return 0;
    }

    LOG_D("hint:%d, duration:%d, pid:%d", hint, duration, pid);
    int hdl, *rsc_list, size = 0;

    Mutex::Autolock lock(sMutex);
    if (getHintRscSize(hint) == -1) {
        return 0;
    }

    if ((rsc_list = (int*)malloc(sizeof(int)*getHintRscSize(hint))) == NULL) {
        ALOGE("Cannot allocate memory for newMsg.rsc");
        return 0;
    }
    perfGetHintRsc(hint, &size, rsc_list);
    if (size == 0) {
        free(rsc_list);
        return 0;
    }

    if (gWorkerExist) {
        hdl = perfLockCheckHdl(0, hint, size, pid, 0, duration);

        pthread_mutex_lock(&g_cond_mutex);
        add_work_req(WORK_MSG_LOCK_ACQ, hdl, size, pid, 0, duration, hint, rsc_list);
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_cond_mutex);
    } else {
        hdl = libpowerhal_LockAcq_internal(rsc_list, 0, size, pid, 0, duration, hint);
    }

    free(rsc_list);

    return hdl;
}

extern "C"
int libpowerhal_CusLockHint_data(int hint, int duration, int pid, int* extendedList, int extendedListSize)
{
    if (!queryAPIstatus(STATUS_CUSLOCKHINT_ENABLED)) {
        LOG_D("libpowerhal_CusLockHint is disabled");
        return 0;
    }

    LOG_D("hint:%d, duration:%d, pid:%d", hint, duration, pid);
    int hdl, *rsc_list, size = 0;

    Mutex::Autolock lock(sMutex);
    if (getHintRscSize(hint) == -1) {
        return 0;
    }

    if ((rsc_list = (int*)malloc(sizeof(int)*(getHintRscSize(hint) + extendedListSize))) == NULL) {
        ALOGE("Cannot allocate memory for newMsg.rsc");
        return 0;
    }

    if (extendedList == NULL || extendedListSize%2 != 0) {
        ALOGE("extendedList is invalid");
        free(rsc_list);
        return 0;
    }

    memcpy(rsc_list, extendedList, sizeof(int)*extendedListSize);

    perfGetHintRsc(hint, &size, rsc_list+extendedListSize);
    if (size == 0) {
        free(rsc_list);
        return 0;
    }

    size += extendedListSize;

    for(int i=0; i<size; i+=2) {
        LOG_V("cus hint data: 0x%x, %d", rsc_list[i], rsc_list[i+1]);
    }

    if (gWorkerExist) {
        hdl = perfLockCheckHdl(0, hint, size, pid, 0, duration);

        pthread_mutex_lock(&g_cond_mutex);
        add_work_req(WORK_MSG_LOCK_ACQ, hdl, size, pid, 0, duration, hint, rsc_list);
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_cond_mutex);
    } else {
        hdl = libpowerhal_LockAcq_internal(rsc_list, 0, size, pid, 0, duration, hint);
    }

    free(rsc_list);

    return hdl;
}

extern "C"
int libpowerhal_LockRel(int hdl)
{
    LOG_D("hdl:%d", hdl);

    Mutex::Autolock lock(sMutex);

    if (gWorkerExist) {
        pthread_mutex_lock(&g_cond_mutex);
        add_work_req(WORK_MSG_LOCK_REL, hdl, 0, 0, 0, 0, 0, NULL);
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_cond_mutex);
    } else {
        hdl = libpowerhal_LockRel_internal(hdl);
    }
    return 0;
}

extern "C"
int libpowerhal_ExitWorker(void)
{
    LOG_E("");

    Mutex::Autolock lock(sMutex);

    if (gWorkerExist) {
        pthread_mutex_lock(&g_cond_mutex);
        add_work_req(WORK_MSG_EXIT, 0, 0, 0, 0, 0, 0, NULL);
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_cond_mutex);
    }
    return 0;
}

extern "C"
int libpowerhal_NotifyAppState(const char *packName, const char *actName, int state, int pid, int uid)
{
    if (!queryAPIstatus(STATUS_APPLIST_ENABLED)) {
        LOG_D("libpowerhal_NotifyAppState is disabled");
        return 0;
    }

    struct tPowerData vPowerData;
    struct tAppStateData vStateData;
    struct tPowerData *vpRspData = NULL;

    Mutex::Autolock lock(sMutex);

    strncpy(vStateData.pack, packName, (MAX_NAME_LEN - 1));
    vStateData.pack[(MAX_NAME_LEN-1)] = '\0';
    strncpy(vStateData.activity, actName, (MAX_NAME_LEN - 1));
    vStateData.activity[(MAX_NAME_LEN-1)] = '\0';
    vStateData.pid = pid;
    vStateData.uid = uid;
    vStateData.state = (int)state;
    vPowerData.msg  = POWER_MSG_NOTIFY_STATE;
    vPowerData.pBuf = (void*)&vStateData;

    LOG_D("");
    power_msg(&vPowerData, (void **) &vpRspData);

    if (vpRspData) {
        if(vpRspData->pBuf)
            free(vpRspData->pBuf);
        free(vpRspData);
    }
    return 0;
}

extern "C"
int libpowerhal_UserGetCapability(int cmd, int id)
{
    Mutex::Autolock lock(sMutex);
    return perfUserGetCapability(cmd, id);
}

extern "C"
int libpowerhal_SetSysInfo(int type, const char *data)
{
    struct tPowerData vPowerData;
    struct tSysInfoData vSysInfoData;
    struct tPowerData *vpRspData = NULL;

    Mutex::Autolock lock(sMutex);

    vSysInfoData.type = type;
    strncpy(vSysInfoData.data, data, (MAX_SYSINFO_LEN - 1));
    vSysInfoData.data[(MAX_SYSINFO_LEN-1)] = '\0';
    vPowerData.msg  = POWER_MSG_SET_SYSINFO;
    vPowerData.pBuf = (void*)&vSysInfoData;

    LOG_D("");
    power_msg(&vPowerData, (void **) &vpRspData);

    if (vpRspData) {
        if(vpRspData->pBuf) {
            vSysInfoData.ret = ((tSysInfoData*)(vpRspData->pBuf))->ret;
            free(vpRspData->pBuf);
        }
        free(vpRspData);
    }
    return vSysInfoData.ret;
}

extern "C"
int libpowerhal_SetScnCallback(int scn, const sp<IMtkPowerCallback> &callback)
{

    Mutex::Autolock lock(sMutex);
    if (callback != nullptr){

        LOG_I(" set scn %x callback", scn);
        perfSetScnUpdateCallback(scn, callback);
    }

    return 0;
}

