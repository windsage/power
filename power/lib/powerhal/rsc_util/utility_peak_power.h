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

#ifndef ANDROID_UTILITY_PEAK_POWER_H
#define ANDROID_UTILITY_PEAK_POWER_H

#include <string>

using namespace std;

#define PATH_PEAK_POWER_MODE "/proc/ppb/peak_power_mode"
#define PATH_PEAK_POWER_CG_MIN_POWER "/proc/ppb/ppb_cg_min_power"
#define PATH_CG_PPT_MODEL_OPTION "/sys/class/cg_ppt/cg_ppt/model_option"
#define PATH_MCUPM_MPMM "/sys/class/misc/mcupm/mpmm"
#define PATH_PT_LB_UT   "/sys/bus/platform/devices/low-battery-throttling/low_battery_protect_ut"
#define PATH_PT_LB_STOP "/sys/bus/platform/devices/low-battery-throttling/low_battery_protect_stop"
#define PATH_PT_OC_UT   "/proc/mtk_batoc_throttling/battery_oc_protect_ut"
#define PATH_PT_OC_STOP "/proc/mtk_batoc_throttling/battery_oc_protect_stop"

extern int set_peak_power_budget_mode(int idx, void *scn);
extern int set_cg_min_power(int idx, void *scn);
extern int set_cg_ppt_model_option_favor_multiscene(int value, void *scn);
extern int unset_cg_ppt_model_option_favor_multiscene(int value, void *scn);
extern int limit_backlight_power(int reduce_b, void *scn);
extern int limit_flashlight_power(int limit_state, void *scn);
extern int limit_wifi_power(int level, void *scn);
extern int set_md_autonomous_ctrl(int on, void *scn);
extern int limit_md_ul_throttle_power(int limit_state, void *scn);
extern int limit_md_txpower_lte_power(int limit_state, void *scn);
extern int limit_md_cc_ctrl_power(int limit_state, void *scn);
extern int unlimit_md_cc_ctrl_power(int limit_state, void *scn);
extern int limit_md_headswitch_power(int limit_state, void *scn);
extern int limit_md_ras_power(int limit_state, void *scn);
extern int limit_md_txpower_fr2_power(int limit_state, void *scn);
extern int limit_md_flightmode_power(int limit_state, void *scn);
extern int limit_md_charger_power(int limit_state, void *scn);
extern int limit_md_txpower_fr1_power(int limit_state, void *scn);
extern int limit_md_uai_power(int limit_state, void *scn);
extern int unlimit_md_uai_power(int limit_state, void *scn);
extern int limit_md_sw_shutdown_power(int limit_state, void *scn);
extern int set_mpmm_enable(int enable, void *scn);
extern int set_pt_enable(int enable, void *scn);

#endif // ANDROID_UTILITY_PEAK_POWER_H

