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

#ifndef __MTKPOWER_HINT_H__
#define __MTKPOWER_HINT_H__

enum {
    //MTK pre-defined hint
    MTKPOWER_HINT_BASE                              = 20,

    MTKPOWER_HINT_PROCESS_CREATE                    = 21,
    MTKPOWER_HINT_PACK_SWITCH                       = 22,
    MTKPOWER_HINT_ACT_SWITCH                        = 23,
    MTKPOWER_HINT_APP_ROTATE                        = 24,
    MTKPOWER_HINT_APP_TOUCH                         = 25,
    MTKPOWER_HINT_GALLERY_BOOST                     = 26,
    MTKPOWER_HINT_GALLERY_STEREO_BOOST              = 27,
    MTKPOWER_HINT_WFD                               = 28,
    MTKPOWER_HINT_PMS_INSTALL                       = 29,
    MTKPOWER_HINT_EXT_LAUNCH                        = 30,
    MTKPOWER_HINT_WHITELIST_LAUNCH                  = 31,
    MTKPOWER_HINT_WIPHY_SPEED_DL                    = 32,
    MTKPOWER_HINT_SDN                               = 33,
    MTKPOWER_HINT_WHITELIST_ACT_SWITCH              = 34,
    MTKPOWER_HINT_BOOT                              = 35,
    MTKPOWER_HINT_AUDIO_LATENCY_DL                  = 36,
    MTKPOWER_HINT_AUDIO_LATENCY_UL                  = 37,
    MTKPOWER_HINT_AUDIO_POWER_DL                    = 38,
    MTKPOWER_HINT_AUDIO_DISABLE_WIFI_POWER_SAVE     = 39,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60     = 40,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90     = 41,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120    = 42,
    MTKPOWER_HINT_UX_SCROLLING                      = 43,
    MTKPOWER_HINT_AUDIO_POWER_UL                    = 44,
    MTKPOWER_HINT_UX_MOVE_SCROLLING                 = 45,
    MTKPOWER_HINT_AUDIO_POWER_HI_RES                = 46,
    MTKPOWER_HINT_GAME_MODE                         = 47,
    MTKPOWER_HINT_VIDEO_MODE                        = 48,
    MTKPOWER_HINT_UX_SCROLLING_COMMON               = 49,
    MTKPOWER_HINT_FRS_ACT                           = 50,
    MTKPOWER_HINT_CAMERA_MODE                       = 51,
    MTKPOWER_HINT_SF_LOW_POWER_MODE                 = 52,
    MTKPOWER_HINT_SCENE_TRANSITION                  = 53,
    MTKPOWER_HINT_UX_TOUCH_MOVE                     = 54,
    MTKPOWER_HINT_AUDIO_HAL_OPEN                    = 55,
    //SPF:fix issues for SBE_LAUNCH_END by sifeng.tian 20241206 start 
    MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE          = 57,
    //SPF:fix issues for SBE_LAUNCH_END by sifeng.tian 20241206 end
    //add for use FramePolicyV3 by wei.kang start
    MTKPOWER_HINT_UX_SCROLLING_COMMON_MASK          = 61,
    //add for use FramePolicyV3 by wei.kang end
    //BSP:add for flingerprint by xiaohong.huang at 20220829 start
    MTKPOWER_HINT_FLINGER_PRINT                     = 75,
    //BSP:add for flingerprint by xiaohong.huang at 20220829 end
    // SDD: add for boost when launcherAnim & inLauncher & startActivityInner & screenOn by chenyu.sun 20240905 start
    MTKPOWER_HINT_LAUNCHER_ANIMA                    = 101,
    MTKPOWER_HINT_SCREEN_ON                         = 102,
    MTKPOWER_HINT_START_ACTIVITY_INNER              = 105,
    MTKPOWER_HINT_IN_LAUNCHER                       = 106,
    // SDD: add for boost when launcherAnim & inLauncher & startActivityInner & screenOn by chenyu.sun 20240905 end
    // SDD: add for boost when file operation by dewang.tan 20241118 start
    MTKPOWER_HINT_FILE_OPERATION                    = 111,
    // SDD: add for boost when file operation by dewang.tan 20241118 end
    //T-HUB Core[SPF]:add for systemui anim by sifeng.tian 20241108 start
    MTKPOWER_HINT_IN_SYSTEMUI                       = 112,
    //T-HUB Core[SPF]:add for systemui anim by sifeng.tian 20241108 end
    MTKPOWER_HINT_LAST,
};

/* IMtkPower::mtkPowerHint */
enum {
    AIDL_BASE = MTKPOWER_HINT_LAST,

//  Boost.aidl
    INTERACTION,
    DISPLAY_UPDATE_IMMINENT,
    ML_ACC,
    AUDIO_LAUNCH,
    CAMERA_LAUNCH,
    CAMERA_SHOT,

//  Mode.aidl
    DOUBLE_TAP_TO_WAKE,
    LOW_POWER,
    SUSTAINED_PERFORMANCE,
    FIXED_PERFORMANCE,
    VR,
    LAUNCH,
    EXPENSIVE_RENDERING,
    INTERACTIVE,
    DEVICE_IDLE,
    DISPLAY_INACTIVE,
    AUDIO_STREAMING_LOW_LATENCY,
    CAMERA_STREAMING_SECURE,
    CAMERA_STREAMING_LOW,
    CAMERA_STREAMING_MID,
    CAMERA_STREAMING_HIGH,
    GAME,
    GAME_LOADING,

    MTKPOWER_HINT_NUM,
};

/* Customer defined hint */
enum{
    CUS_HINT_BASE = 1001,

    CUS_HINT_MAX = 10000,
};

enum{
    MTKPOWER_HINT_ALWAYS_ENABLE = 0x0FFFFFFF,
};

#endif // __MTKPOWER_HINT_H__
