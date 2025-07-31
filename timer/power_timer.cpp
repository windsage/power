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

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_TAG "power@timer"

#include <log/log.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
//#include <sys/select.h>
#include <sys/time.h>
#include <sys/eventfd.h>
//#include "eara_util.h"
#include "power_timer.h"
#include "ptimer.h"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

#define MAX_TIMER_COUNT     64

struct tTimer {
    int used;
    int idx;
    void *p_pTimer;
    timer_callback timer_cb;
};

struct tTimer powerTimer[MAX_TIMER_COUNT];
static void * _gpTimerMng;
static int gEventFd;

static void init_timer(void)
{
    int i;

    ptimer_mng_create(&_gpTimerMng);
    gEventFd = eventfd(0, 0);

    for(i=0; i<MAX_TIMER_COUNT; i++) {
        powerTimer[i].used = 0;
        powerTimer[i].idx = i;
        powerTimer[i].p_pTimer = NULL;
        powerTimer[i].timer_cb = NULL;
    }
}

static int allocate_timer(void)
{
    int i;
    for (i=0; i < MAX_TIMER_COUNT; i++) {
        if (powerTimer[i].used == 0) {
            powerTimer[i].used = 1;
            return i;
        }
    }
    return -1;
}

static int find_timer(timer_callback timer_cb)
{
    int idx = -1, i;

    for (i=0; i < MAX_TIMER_COUNT; i++) {
        if (powerTimer[i].used == 1 && powerTimer[i].timer_cb == timer_cb) {
            idx = i;
            break;
        }
    }
    return idx;
}

static int reset_timer(int i)
{
    if(i < 0 || i >= MAX_TIMER_COUNT)
        return -1;

    powerTimer[i].used = 0;
    powerTimer[i].p_pTimer = NULL;
    powerTimer[i].timer_cb = NULL;
    return 0;
}

static int powerTimerHandler(const void * pTimer, void * pData)
{
    struct tTimer *ptTimer = (struct tTimer *)pData;

    LOG_D("");

    if(ptTimer->p_pTimer != pTimer)
        return -1;

    if(ptTimer->timer_cb) {
        ptTimer->timer_cb(0);
    }

    LOG_D("ptTimer->idx:%d", ptTimer->idx);

    ptimer_delete(ptTimer->p_pTimer);
    reset_timer(ptTimer->idx);
    return 0;
}

static void* mtkPowerTimer(void *data)
{
    LOG_V("%p", data);
    struct timeval vtv;
    int vRet;
    unsigned long vmsec;
    unsigned long vWallSec;
    unsigned long vWallNSec;
    void * pTimer = NULL;
    void * pData = NULL;
    fd_set vRSet, vWSet;
    uint64_t event;
    int visRead;

    while(1) {

        FD_ZERO(&vRSet);
        FD_SET(gEventFd, &vRSet);

        vRet = ptimer_mng_getnextduration(_gpTimerMng, &vmsec);
        //LOG_I("vRet 1:%d", vRet);

        if (vRet != 0) {
            vtv.tv_sec = 60;
            vtv.tv_usec = 0;
        } else {
            vtv.tv_usec = (vmsec % 1000) * 1000;
            vtv.tv_sec = vmsec / 1000;
            LOG_D("vmsec:%ld", vmsec);
        }

        select(gEventFd+1, &vRSet, NULL, NULL, &vtv);
        visRead = FD_ISSET(gEventFd, &vRSet);
        LOG_D("visRead:%d", visRead);

        if (visRead) {
            read(gEventFd, &event, sizeof(uint64_t));
        }

        //if (vRet == 0) {
            vRet = ptimer_mng_getexpired(_gpTimerMng, &pTimer, &pData, &vWallSec, &vWallNSec);
            if (vRet == 1) {
                powerTimerHandler(pTimer, pData);
            }

            vRet = ptimer_mng_getexpired(_gpTimerMng, &pTimer, &pData, &vWallSec, &vWallNSec);
            if (vRet == 1) {
                powerTimerHandler(pTimer, pData);
            }
        //}
    }

    return NULL;
}


/*
    Public API
 */
int createTimerThread(pthread_t *thread)
{
    pthread_attr_t attr;

    init_timer();

    if(thread == NULL)
        return -1;

    /* handler */
    pthread_attr_init(&attr);
    pthread_create(thread, &attr, mtkPowerTimer, NULL);
    pthread_setname_np(*thread, "mtkPowerTimer");

    return 0;
}

int addTimerCallback(int timeout, timer_callback timer_cb)
{
    int idx;
    uint64_t event = 1;

    LOG_D("timeout:%d", timeout);

    idx = allocate_timer();

    if (idx < 0 || idx >=  MAX_TIMER_COUNT)
        return -1;

    LOG_D("timer idx:%d", idx);
    if (idx >= 0 && idx < MAX_TIMER_COUNT) {
        powerTimer[idx].timer_cb = timer_cb;
        ptimer_create(&(powerTimer[idx].p_pTimer));
        ptimer_start(_gpTimerMng, powerTimer[idx].p_pTimer, timeout, &(powerTimer[idx]));

        write(gEventFd, &event, sizeof(uint64_t));
    }

    return 0;
}

int cancelTimerCallback(timer_callback timer_cb)
{
    int idx;

    idx = find_timer(timer_cb);

    if (idx < 0 || idx >=  MAX_TIMER_COUNT)
        return -1;

    if (idx >= 0 && idx < MAX_TIMER_COUNT) {
        ptimer_stop(powerTimer[idx].p_pTimer);
        ptimer_delete(powerTimer[idx].p_pTimer);
        reset_timer(idx);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

