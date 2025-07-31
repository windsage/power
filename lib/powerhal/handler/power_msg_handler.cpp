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

#define LOG_TAG "libPowerHal_msg"

#include <pthread.h>
#include <utils/Log.h>
#include <sys/resource.h>

#include "power_msg_handler.h"
#include "powerd_int.h"
#include "power_util.h"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)
#define MSG_THREAD_SCHED_PRIORITY -20

static void wakeupService(void)
{
    return;
}

static void* mtkPowerMsgHandler(void *data)
{
    LOG_V("%p", data);
    if (::setpriority(PRIO_PROCESS, 0, MSG_THREAD_SCHED_PRIORITY) != 0) {
        ALOGI("fail to set power msg nice value:%d", MSG_THREAD_SCHED_PRIORITY);
    }
    ALOGI("Success to set power msg nice value:%d", MSG_THREAD_SCHED_PRIORITY);
    powerd_core_pre_init();
    powerd_main(0, NULL, wakeupService);
    return NULL;
}

static int startPowerMsgTimer(int type, int data, int duration)
{
    struct tPowerData vPowerData;
    struct tPowerData vRspData;
    struct tTimerData vTimerData;

    vTimerData.type = type;
    vTimerData.data = data;
    vTimerData.duration = duration;
    vPowerData.msg  = POWER_MSG_TIMER_REQ;
    vPowerData.pBuf = (void*)&vTimerData;
    vRspData.pBuf = (void*)&vTimerData;

    ALOGI("%s %d,%d,%d", __func__, type, data, duration);
    power_msg_timer_req(&vPowerData, (void **) &vRspData);
    return 0;
}

int createPowerMsgHandler()
{
    pthread_t handlerThread;
    pthread_attr_t attr;

    /* handler */
    pthread_attr_init(&attr);
    pthread_create(&handlerThread, &attr, mtkPowerMsgHandler, NULL);
    pthread_setname_np(handlerThread, "mtkPowerMsgHdl");

    return 0;
}

int setPerfLockDuration(int hdl, int duration)
{
    return startPowerMsgTimer(TIMER_PERF_LOCK_DURATION, hdl, duration);
}

int setHintDuration(int hint, int duration)
{
    return startPowerMsgTimer(TIMER_HINT_DURATION, hint, duration);
}

