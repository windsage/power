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

#define LOG_TAG "libPowerHal_boot"

#include <pthread.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "power_boot_handler.h"
#include "mtkpower_hint.h"
#include "perfservice.h"
#include "perfservice_prop.h"
#include "powerhal_api.h"

#define BOOT_BOOST_POLL_COUNT (60)
#define BOOT_BOOST_POLL_TIME  (3) // 3 sec.


static void* mtkPowerBootHandler(void *data)
{
    int hdl;
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    int  prop_value = 0;
    int timeout = BOOT_BOOST_POLL_COUNT;

    LOG_I("boot boost, %p", data);

    /* it should run after powerhal init */
    sleep(1);
    hdl = libpowerhal_CusLockHint(MTKPOWER_HINT_BOOT,
        BOOT_BOOST_POLL_COUNT * BOOT_BOOST_POLL_TIME * 1000, (int)getpid());

    while (timeout > 0) {
        property_get(POWER_PROP_BOOT_COMPLETE, prop_content, "0"); // init before ?
        prop_value = atoi(prop_content);
        if (prop_value == 1) {
            LOG_I("boot complete");
            break;
        }
        sleep(BOOT_BOOST_POLL_TIME);
        timeout--;
    }

    libpowerhal_LockRel(hdl);
    LOG_I("boot boost exit");

    // enable powerhal api after boot complete
    setAPIstatus(STATUS_PERFLOCKACQ_ENABLED, 1);
    setAPIstatus(STATUS_CUSLOCKHINT_ENABLED, 1);
    setAPIstatus(STATUS_APPLIST_ENABLED, 1);
    LOG_I("Enable PowerHAL API after boot completed.");

    return NULL;
}

int createBootHandler()
{
    pthread_t handlerThread;
    pthread_attr_t attr;

    /* handler */
    pthread_attr_init(&attr);
    pthread_create(&handlerThread, &attr, mtkPowerBootHandler, NULL);
    pthread_setname_np(handlerThread, "mtkPowerBoot");

    return 0;
}

