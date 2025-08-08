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

#define LOG_TAG "mtkpower_client"

#define PATH_TURBO_FOREGROUND_INFO_NOTIFY          "/sys/module/task_turbo/parameters/update_win_pid_status"
#define PATH_VIP_ENGINE_FOREGROUND_INFO_NOTIFY     "/sys/module/vip_engine/parameters/update_win_pid_status"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <log/log.h>
#include <inttypes.h>
#include <vector>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <aidl/vendor/mediatek/hardware/mtkpower_applist/IMtkpower_applist.h>
#include <common.h>
#include "utility_task_turbo.h"

typedef struct ForegroundAPP {
    int pid = -1;
    int actNum = 0;
    char packname[PACK_NAME_MAX] = "\0";
    Activity activity[FOUGROUND_APP_MAX_NUM];
} ForegroundAPP;

static ForegroundAPP foregroundAppList[FOUGROUND_APP_MAX_NUM];
static bool turbo_win_pid_support = false;
static bool vip_engine_win_pid_support = false;

static bool check_task_turbo_support();

static bool check_task_turbo_support() {
    static bool hasChecked = false;

    if(!hasChecked) {
        if(access(PATH_TURBO_FOREGROUND_INFO_NOTIFY, W_OK) != -1) {
            turbo_win_pid_support = true;
            ALOGI("Access %s successfully!", PATH_TURBO_FOREGROUND_INFO_NOTIFY);
        } else {
            turbo_win_pid_support = false;
            ALOGE("Access %s error!", PATH_TURBO_FOREGROUND_INFO_NOTIFY);
        }

        if(access(PATH_VIP_ENGINE_FOREGROUND_INFO_NOTIFY, W_OK) != -1) {
            vip_engine_win_pid_support = true;
            ALOGI("Access %s successfully!", PATH_VIP_ENGINE_FOREGROUND_INFO_NOTIFY);
        } else {
            vip_engine_win_pid_support = false;
            ALOGE("Access %s error!", PATH_VIP_ENGINE_FOREGROUND_INFO_NOTIFY);
        }
        hasChecked = true;
    }

    return turbo_win_pid_support || vip_engine_win_pid_support;
}

static int addForegroundAPP(const char* packname, const char* actname, int pid, uint32_t activityId) {
    for(int i=0; i<FOUGROUND_APP_MAX_NUM; i++) {
        if(foregroundAppList[i].pid == pid) {
            int exist = 0;
            for(int j=0; j<FOUGROUND_APP_MAX_NUM; j++) {
                if(foregroundAppList[i].activity[j].actId == activityId) {
                    exist = 1;
                    ALOGD("%s: FG app exist : %s/%s, %d, %d", __FUNCTION__,
                        foregroundAppList[i].packname, foregroundAppList[i].activity[j].actname, i, j);
                    break;
                }
            }

            if(exist) return 0;

            for(int j=0; j<FOUGROUND_APP_MAX_NUM; j++) {
                if(foregroundAppList[i].activity[j].actId == -1) {
                    foregroundAppList[i].activity[j].actId = activityId;
                    strncpy(foregroundAppList[i].activity[j].actname, actname, PACK_NAME_MAX-1);
                    foregroundAppList[i].actNum++;
                    ALOGD("%s: Add activity success : %s/%s, %d, %d", __FUNCTION__,
                        foregroundAppList[i].packname, foregroundAppList[i].activity[j].actname, i, j);
                }
            }

            return 1;
        }
    }

    for(int i=0; i<FOUGROUND_APP_MAX_NUM; i++) {
        if(foregroundAppList[i].pid == -1) {
            foregroundAppList[i].pid = pid;
            strncpy(foregroundAppList[i].packname, packname, PACK_NAME_MAX-1);
            foregroundAppList[i].activity[0].actId = activityId;
            strncpy(foregroundAppList[i].activity[0].actname, actname, PACK_NAME_MAX-1);
            foregroundAppList[i].actNum++;
            ALOGD("%s: Add FG app success : %s/%s, %d", __FUNCTION__,
                foregroundAppList[i].packname, foregroundAppList[i].activity[0].actname, i);

            return 1;
        }
    }

    ALOGE("%s: Add FG app failed!!!", __FUNCTION__);

    return 0;
}

static int removeForegroundAPP(const char* packname, const char* actname, int pid, uint32_t activityId) {
    int i, j;
    for(i=0; i<FOUGROUND_APP_MAX_NUM; i++) {
        if(foregroundAppList[i].pid == pid) {
            for(j=0; j<FOUGROUND_APP_MAX_NUM; j++) {
                if(foregroundAppList[i].activity[j].actId == activityId) {
                    foregroundAppList[i].activity[j].actId = -1;
                    strncpy(foregroundAppList[i].activity[j].actname, "\0", 1);
                    foregroundAppList[i].actNum--;
                    ALOGD("%s: Remove activity success : %s/%s, %d, %d", __FUNCTION__,
                        foregroundAppList[i].packname, actname, i, j);
                }
            }

            if(foregroundAppList[i].actNum == 0) {
                foregroundAppList[i].pid = -1;
                ALOGD("%s: Remove FG app success : %s, %d", __FUNCTION__,
                        foregroundAppList[i].packname, i);
            }
        }
    }

    return 0;
}

static int checkForegroundAPP(int pid) {
    for(int i=0; i<FOUGROUND_APP_MAX_NUM; i++) {
        if(foregroundAppList[i].pid == pid) {
            return 1;
        }
    }

    return 0;
}

int taskTurboNotifyForegroundApp(const char *packname, const char *actname, uint32_t pid, uint32_t activityId,
                                int32_t status, uint32_t uid) {
    if(!check_task_turbo_support())
        return -1;

    ALOGD("[taskTurboNotifyForegroundApp] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);

    if (status == STATE_RESUME) {
        addForegroundAPP(packname, actname, pid, activityId);
        // Support VIP Engine in task turbo
        if (turbo_win_pid_support && checkForegroundAPP(pid)) {
            int ret = set_value(PATH_TURBO_FOREGROUND_INFO_NOTIFY, pid, status);
            if(ret == 0) {
                ALOGE("%s: notify win pid error! %d", __FUNCTION__, ret);

                return -1;
            }
            ALOGD("%s: win_pid_status, value: %d %d", __FUNCTION__, pid, status);
        }
        // Support VIP Engine isolation
        if (vip_engine_win_pid_support && checkForegroundAPP(pid)) {
            int ret = set_value(PATH_VIP_ENGINE_FOREGROUND_INFO_NOTIFY, pid, status);
            if(ret == 0) {
                ALOGE("%s: notify win pid error! %d", __FUNCTION__, ret);

                return -1;
            }
            ALOGD("%s: win_pid_status, value: %d %d", __FUNCTION__, pid, status);
        }
    }else if (status == STATE_PAUSE || status == STATE_DEAD) {
        removeForegroundAPP(packname, actname, pid, activityId);
        // Support VIP Engine in task turbo
        if (turbo_win_pid_support && !checkForegroundAPP(pid)) {
            int ret = set_value(PATH_TURBO_FOREGROUND_INFO_NOTIFY, pid, status);
            if(ret == 0) {
                ALOGE("%s: notify win pid error! %d", __FUNCTION__, ret);

                return -1;
            }
            ALOGD("%s: notify win_pid_status, value: %d %d", __FUNCTION__, pid, status);
        }
        // Support VIP Engine isolation
        if (vip_engine_win_pid_support && !checkForegroundAPP(pid)) {
            int ret = set_value(PATH_VIP_ENGINE_FOREGROUND_INFO_NOTIFY, pid, status);
            if(ret == 0) {
                ALOGE("%s: notify win pid error! %d", __FUNCTION__, ret);

                return -1;
            }
            ALOGD("%s: win_pid_status, value: %d %d", __FUNCTION__, pid, status);
        }
    }

    return 0;
}