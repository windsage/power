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

#define LOG_TAG "libPowerHal"

#include <pthread.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include "power_error_handler.h"
#include "mtkpower_hint.h"
#include "perfservice.h"
#include "perfservice_prop.h"
#include "powerhal_api.h"
#include "mtkperf_resource.h"

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int error_event = 0;

void notifyWriteFail()
{
    LOG_E("");
    error_event = 1;
    pthread_mutex_lock(&mut);
    if (error_event) {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mut);
}

int retry_Cpufreq()
{
    LOG_E("");
    sleep(1);

    int perf_lock_rsc[] = {PERF_RES_CPUFREQ_MAX_CLUSTER_0, 0};
    int size = sizeof(perf_lock_rsc)/sizeof(int);
    int hdl = 0;
    hdl = libpowerhal_LockAcq(perf_lock_rsc, hdl, size, (int)getpid(), size, 1);

    return 0;
}

static void* powerhal_error_handler(void *data)
{
    while (1) {
        pthread_mutex_lock(&mut);
        if (!error_event) {
            pthread_cond_wait(&cond, &mut);
        }
        error_event = 0;
        pthread_mutex_unlock(&mut);

        retry_Cpufreq();
    }
    return NULL;
}

int create_error_handler()
{
    pthread_t handlerThread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&handlerThread, &attr, powerhal_error_handler, NULL);
    pthread_setname_np(handlerThread, "PowerErrHandler");

    return 0;
}