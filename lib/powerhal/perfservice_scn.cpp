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

#include <utils/Log.h>
#include <string>
#include <vector>
#include "mtkpower_hint.h"
#include "mtkperf_resource.h"
#include "mtkpower_types.h"

using std::string;
using std::vector;

typedef struct tHintMap {
    int hintId;
    string hintCmd;
    int lock_rsc_size;
    int lock_rsc_list[MAX_ARGS_PER_REQUEST];
} tHintMap;

vector<tHintMap> PowerScnTbl;

tHintMap HintMappingTbl[] = {

    //Boost

    {
        .hintId     = INTERACTION,
        .hintCmd    = "INTERACTION",
    },

    {
        .hintId     = DISPLAY_UPDATE_IMMINENT,
        .hintCmd    = "DISPLAY_UPDATE_IMMINENT",
    },

    {
        .hintId     = ML_ACC,
        .hintCmd    = "ML_ACC",
    },

    {
        .hintId     = AUDIO_LAUNCH,
        .hintCmd    = "AUDIO_LAUNCH",
    },

    {
        .hintId     = CAMERA_LAUNCH,
        .hintCmd    = "CAMERA_LAUNCH",
    },

    {
        .hintId     = CAMERA_SHOT,
        .hintCmd    = "CAMERA_SHOT",
    },

    //Mode

    {
        .hintId     = DOUBLE_TAP_TO_WAKE,
        .hintCmd    = "DOUBLE_TAP_TO_WAKE",
    },

    {
        .hintId     = LOW_POWER,
        .hintCmd    = "LOW_POWER",
    },

    {
        .hintId     = SUSTAINED_PERFORMANCE ,
        .hintCmd    = "SUSTAINED_PERFORMANCE",
    },

    {
        .hintId     = FIXED_PERFORMANCE,
        .hintCmd    = "FIXED_PERFORMANCE",
    },

    {
        .hintId     = VR,
        .hintCmd    = "VR",
    },

    {
        .hintId     = LAUNCH,
        .hintCmd    = "LAUNCH",
    },

    {
        .hintId     = EXPENSIVE_RENDERING,
        .hintCmd    = "EXPENSIVE_RENDERING",
    },

    {
        .hintId     = INTERACTIVE,
        .hintCmd    = "INTERACTIVE",
    },

    {
        .hintId     = DEVICE_IDLE,
        .hintCmd    = "DEVICE_IDLE ",
    },

    {
        .hintId     = DISPLAY_INACTIVE,
        .hintCmd    = "DISPLAY_INACTIVE",
    },

    {
        .hintId     = AUDIO_STREAMING_LOW_LATENCY,
        .hintCmd    = "AUDIO_STREAMING_LOW_LATENCY",
    },

    {
        .hintId     = CAMERA_STREAMING_SECURE,
        .hintCmd    = "CAMERA_STREAMING_SECURE",
    },

    {
        .hintId     = CAMERA_STREAMING_LOW,
        .hintCmd    = "CAMERA_STREAMING_LOW",
    },

    {
        .hintId     = CAMERA_STREAMING_MID,
        .hintCmd    = "CAMERA_STREAMING_MID",
    },

    {
        .hintId     = CAMERA_STREAMING_HIGH,
        .hintCmd    = "CAMERA_STREAMING_HIGH",
    },

    {
        .hintId     = GAME,
        .hintCmd    = "GAME",
    },

    {
        .hintId     = GAME_LOADING,
        .hintCmd    = "GAME_LOADING",
    },

    {
        .hintId     = DISPLAY_CHANGE,
        .hintCmd    = "DISPLAY_CHANGE",
    },

    {
        .hintId     = AUTOMOTIVE_PROJECTION,
        .hintCmd    = "AUTOMOTIVE_PROJECTION",
    },

    //MTK powerhint

    {
        .hintId     = MTKPOWER_HINT_PROCESS_CREATE,
        .hintCmd    = "MTKPOWER_HINT_PROCESS_CREATE",
    },

    {
        .hintId     = MTKPOWER_HINT_PACK_SWITCH,
        .hintCmd    = "MTKPOWER_HINT_PACK_SWITCH",
    },

    {
        .hintId     = MTKPOWER_HINT_ACT_SWITCH,
        .hintCmd    = "MTKPOWER_HINT_ACT_SWITCH",
    },

    {
        .hintId     = MTKPOWER_HINT_APP_ROTATE,
        .hintCmd    = "MTKPOWER_HINT_APP_ROTATE",
    },

    {
        .hintId     = MTKPOWER_HINT_APP_TOUCH,
        .hintCmd    = "MTKPOWER_HINT_APP_TOUCH",
    },

    {
        .hintId     = MTKPOWER_HINT_GALLERY_BOOST,
        .hintCmd    = "MTKPOWER_HINT_GALLERY_BOOST",
    },

    {
        .hintId     = MTKPOWER_HINT_GALLERY_STEREO_BOOST,
        .hintCmd    = "MTKPOWER_HINT_GALLERY_STEREO_BOOST",
    },

    {
        .hintId     = MTKPOWER_HINT_WFD,
        .hintCmd    = "MTKPOWER_HINT_WFD",
    },

    {
        .hintId     = MTKPOWER_HINT_PMS_INSTALL,
        .hintCmd    = "MTKPOWER_HINT_PMS_INSTALL",
    },

    {
        .hintId     = MTKPOWER_HINT_EXT_LAUNCH,
        .hintCmd    = "MTKPOWER_HINT_EXT_LAUNCH",
    },

    {
        .hintId     = MTKPOWER_HINT_WHITELIST_LAUNCH,
        .hintCmd    = "MTKPOWER_HINT_WHITELIST_LAUNCH",
    },

    {
        .hintId     = MTKPOWER_HINT_WIPHY_SPEED_DL,
        .hintCmd    = "MTKPOWER_HINT_WIPHY_SPEED_DL",
    },

    {
        .hintId     = MTKPOWER_HINT_SDN,
        .hintCmd    = "MTKPOWER_HINT_SDN",
    },

    {
        .hintId     = MTKPOWER_HINT_WHITELIST_ACT_SWITCH,
        .hintCmd    = "MTKPOWER_HINT_WHITELIST_ACT_SWITCH",
    },

    {
        .hintId     = MTKPOWER_HINT_BOOT,
        .hintCmd    = "MTKPOWER_HINT_BOOT",
    },

    {
        .hintId     = MTKPOWER_HINT_AUDIO_LATENCY_DL,
        .hintCmd    = "MTKPOWER_HINT_AUDIO_LATENCY_DL",
    },

    {
        .hintId     = MTKPOWER_HINT_AUDIO_LATENCY_UL,
        .hintCmd    = "MTKPOWER_HINT_AUDIO_LATENCY_UL",
    },

    {
        .hintId     = MTKPOWER_HINT_AUDIO_POWER_DL,
        .hintCmd    = "MTKPOWER_HINT_AUDIO_POWER_DL",
    },

    {
        .hintId     = MTKPOWER_HINT_AUDIO_DISABLE_WIFI_POWER_SAVE,
        .hintCmd    = "MTKPOWER_HINT_AUDIO_DISABLE_WIFI_POWER_SAVE",
    },

    {
        .hintId     = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60,
        .hintCmd    = "MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60",
    },

    {
        .hintId     = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90,
        .hintCmd    = "MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90",
    },

    {
        .hintId     = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120,
        .hintCmd    = "MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120",
    },

    {
        .hintId     = MTKPOWER_HINT_UX_SCROLLING,
        .hintCmd    = "MTKPOWER_HINT_UX_SCROLLING",
    },

    {
        .hintId     = MTKPOWER_HINT_AUDIO_POWER_UL,
        .hintCmd    = "MTKPOWER_HINT_AUDIO_POWER_UL",
    },

    {
        .hintId     = MTKPOWER_HINT_GAME_MODE,
        .hintCmd    = "MTKPOWER_HINT_GAME_MODE",
    },

    {
        .hintId     = MTKPOWER_HINT_VIDEO_MODE,
        .hintCmd    = "MTKPOWER_HINT_VIDEO_MODE",
    },

    {
        .hintId     = MTKPOWER_HINT_UX_MOVE_SCROLLING,
        .hintCmd    = "MTKPOWER_HINT_UX_MOVE_SCROLLING",
    },

    {
        .hintId     = MTKPOWER_HINT_UX_SCROLLING_COMMON,
        .hintCmd    = "MTKPOWER_HINT_UX_SCROLLING_COMMON",
    },

    {
        .hintId     = MTKPOWER_HINT_UX_SCROLLING_COMMON_MASK,
        .hintCmd    = "MTKPOWER_HINT_UX_SCROLLING_COMMON_MASK",
    },

    {
        .hintId     = MTKPOWER_HINT_FRS_ACT,
        .hintCmd    = "MTKPOWER_HINT_FRS_ACT",
    },

    {
        .hintId     = MTKPOWER_HINT_CAMERA_MODE,
        .hintCmd    = "MTKPOWER_HINT_CAMERA_MODE",
    },

    {
        .hintId     = MTKPOWER_HINT_SF_LOW_POWER_MODE,
        .hintCmd    = "MTKPOWER_HINT_SF_LOW_POWER_MODE",
    },

    {
        .hintId     = MTKPOWER_HINT_SCENE_TRANSITION,
        .hintCmd    = "MTKPOWER_HINT_SCENE_TRANSITION",
    },

    {
        .hintId     = MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE_GAME,
        .hintCmd    = "MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE_GAME",
    },

    {
        .hintId     = MTKPOWER_HINT_EXT_LAUNCH_FOR_GAME,
        .hintCmd    = "MTKPOWER_HINT_EXT_LAUNCH_FOR_GAME",
    },

    {
        .hintId     = MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE,
        .hintCmd    = "MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE",
    },

    {
        .hintId     = MTKPOWER_HINT_PACK_SWITCH_ANIMATION_OFF,
        .hintCmd    = "MTKPOWER_HINT_PACK_SWITCH_ANIMATION_OFF",
    },

    {
        .hintId     = MTKPOWER_HINT_EXT_ACT_SWITCH,
        .hintCmd    = "MTKPOWER_HINT_EXT_ACT_SWITCH",
    },

    {
        .hintId     = MTKPOWER_HINT_EXT_LAUNCH_ANIMATION_OFF,
        .hintCmd    = "MTKPOWER_HINT_EXT_LAUNCH_ANIMATION_OFF",
    },

    {
        .hintId     = MTKPOWER_HINT_UX_TOUCH_MOVE,
        .hintCmd    = "MTKPOWER_HINT_UX_TOUCH_MOVE",
    },

    {
        .hintId     = MTKPOWER_HINT_AUDIO_ROUTING,
        .hintCmd    = "MTKPOWER_HINT_AUDIO_ROUTING",
    },

    {
        .hintId     = MTKPOWER_HINT_PROCESS_CREATE_BALANCE_MODE_GAME,
        .hintCmd    = "MTKPOWER_HINT_PROCESS_CREATE_BALANCE_MODE_GAME",
    },

    {
        .hintId     = MTKPOWER_HINT_HOT_LAUNCH,
        .hintCmd    = "MTKPOWER_HINT_HOT_LAUNCH",
    },
};


int getHintId(const char *cmd)
{
    int i, tblLen;
    tblLen = sizeof(HintMappingTbl) / sizeof(tHintMap);

    ALOGD("getHintId: %s", cmd);

    for(i=0; i<tblLen; i++) {
        if(strncmp(HintMappingTbl[i].hintCmd.c_str(), cmd, 128) == 0) {
            ALOGD("getHintId: %s, id:%d", cmd, HintMappingTbl[i].hintId);
            return HintMappingTbl[i].hintId;
        }
    }
    return -1;
}

void getHintCmd(int hint, char *str)
{
    int i, tblLen;
    tblLen = sizeof(HintMappingTbl) / sizeof(tHintMap);

    for(i=0; i<tblLen; i++) {
        if(HintMappingTbl[i].hintId == hint) {
            strncpy(str, HintMappingTbl[i].hintCmd.c_str(), HintMappingTbl[i].hintCmd.length());
            return;
        }
    }
    return;
}

string getHintName(int hint)
{
    int i;
    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId)
            return PowerScnTbl[i].hintCmd;
    }

    ALOGE("[getHintName] hint_id %d cannot be found in PowerScnTbl", hint);
    return "";
}

int getHintRscSize(int hint)
{
    int i;
    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId)
            return PowerScnTbl[i].lock_rsc_size;
    }

    ALOGE("[getHintRscSize] hint_id %d cannot be found in PowerScnTbl", hint);
    return -1;
}

int getHintRscElement(int hint, int idx)
{
    int i;
    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId)
            return PowerScnTbl[i].lock_rsc_list[idx];
    }

    ALOGE("[getHintRscElement] hint_id %d cannot be found in PowerScnTbl", hint);
    return -1;
}

int * getHintRscList(int hint)
{
    int i;
    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId)
            return PowerScnTbl[i].lock_rsc_list;
    }

    ALOGE("[getHintRscList] hint_id %d cannot be found in PowerScnTbl", hint);
    return NULL;
}

int getHintRscListParam(int hint, int cmd)
{
    int i;
    int idx_hint = 0;
    int param = -1;

    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId) {
            idx_hint = i;
        }
    }

    if (!idx_hint) {
        for (i = 0; i < PowerScnTbl[i].lock_rsc_size; i++) {
            if (PowerScnTbl[idx_hint].lock_rsc_list[i] == cmd) {
                param = PowerScnTbl[idx_hint].lock_rsc_list[i+1];
                return param;
            }
        }
    }

    return param;
}

void HintRscList_remove(int hint, int cmd)
{
    int i;
    int idx_hint = -1;

    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId) {
            idx_hint = i;
            ALOGD("[HintRscList_remove] hint_id %d cmd = %s ", idx_hint, PowerScnTbl[idx_hint].hintCmd.c_str());
        }
    }
    if (idx_hint > 0) {
        for (i = 0; i < PowerScnTbl[idx_hint].lock_rsc_size; i++) {
            if (PowerScnTbl[idx_hint].lock_rsc_list[i] == cmd) {
                PowerScnTbl[idx_hint].lock_rsc_list[i] = 0;
                PowerScnTbl[idx_hint].lock_rsc_list[i+1] = 0;
                ALOGD("[HintRscList_remove] cmd: %d", cmd);
                return;
            }
        }
    }
    return;
}

void HintRscList_modify(int hint, int cmd, int new_param)
{
    int i;
    int idx_hint = -1;

    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId) {
            idx_hint = i;
            ALOGD("[HintRscList_modify] hint_id %d cmd = %s ", idx_hint, PowerScnTbl[idx_hint].hintCmd.c_str());
        }
    }
    if (idx_hint > 0) {
        for (i = 0; i < PowerScnTbl[idx_hint].lock_rsc_size; i++) {
            if (PowerScnTbl[idx_hint].lock_rsc_list[i] == cmd) {
                PowerScnTbl[idx_hint].lock_rsc_list[i+1] = new_param;
                ALOGD("[HintRscList_modify] cmd: %d new: %d ", cmd, new_param);
                return;
            }
        }
    }
    return;
}

void HintRscList_append(int hint, int cmd, int param_1)
{
    int i, idx = 0;
    for (i = 0; i < PowerScnTbl.size(); i++ ) {
        if (hint == PowerScnTbl[i].hintId) {
            idx = PowerScnTbl[i].lock_rsc_size;
            PowerScnTbl[i].lock_rsc_list[idx] = cmd;
            PowerScnTbl[i].lock_rsc_list[idx+1] = param_1;
            PowerScnTbl[i].lock_rsc_size += 2;
            return;
        }
    }

    ALOGE("[HintRscList_append] hint_id %d cannot be found in PowerScnTbl", hint);
    return;
}

void HintRscList_init()
{
    int i, j;
    int tblLen = sizeof(HintMappingTbl) / sizeof(tHintMap);
    for (i = 0; i < tblLen; i++) {
        HintMappingTbl[i].lock_rsc_size = 0;
        for (j = 0; j < MAX_ARGS_PER_REQUEST; j++) {
            HintMappingTbl[i].lock_rsc_list[j] = 0;
        }
        PowerScnTbl.push_back(HintMappingTbl[i]);
    }
    ALOGI("PowerScnTbl.size: %d", PowerScnTbl.size());
}

void PowerScnTbl_append(const char* hintname, int hint_id)
{
    int i;
    tHintMap temp;
    if (hintname != nullptr)
        temp.hintCmd = hintname;
    else
        temp.hintCmd = " ";
    temp.hintId = hint_id;
    temp.lock_rsc_size = 0;
    for (i = 0; i < MAX_ARGS_PER_REQUEST; i++)
        temp.lock_rsc_list[i] = 0;

    PowerScnTbl.push_back(temp);
    ALOGI("append hint_id: %d,  PowerScnTbl.size: %d", hint_id, PowerScnTbl.size());
}
