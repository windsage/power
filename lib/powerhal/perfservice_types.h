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

#ifndef ANDROID_PERFSERVICE_TYPES_H
#define ANDROID_PERFSERVICE_TYPES_H

#include "mtkperf_resource.h"

#define PACK_NAME_MAX   128
#define CLASS_NAME_MAX  128
#define CLUSTER_MAX     8
#define COMM_NAME_SIZE  64
#define FIELD_SIZE      1024
#define SBE_NAME_MAX    128

typedef struct tScnNode{
    int  handle_idx;
    int  scn_type;
    int  scn_state;
    char pack_name[PACK_NAME_MAX];
    char act_name[CLASS_NAME_MAX];
    char sbe_featurename[SBE_NAME_MAX];
    char fps[CLASS_NAME_MAX];
    char window_mode[CLASS_NAME_MAX];
    int  scn_core_total;
    int  scn_core_min[CLUSTER_MAX];
    int  scn_core_max[CLUSTER_MAX];
    int  scn_freq_min[CLUSTER_MAX];
    int  scn_freq_max[CLUSTER_MAX];
    int  scn_freq_hard_min[CLUSTER_MAX];
    int  scn_freq_hard_max[CLUSTER_MAX];
    int  scn_gpu_freq;
    int  scn_gpu_freq_max;    // upper bound
    int  scn_gpu_freq_hard_min;
    int  scn_gpu_freq_hard_max;
    int  render_pid;
    int  hint_hold_time;
    int  ext_hint;
    int  ext_hint_hold_time;
    int  launch_time_cold;
    int  launch_time_warm;
    int  act_switch_time;
    int  screen_off_action;
    int  pid;
    int  tid;
    int  scn_rsc[FIELD_SIZE];
    int  scn_prev_rsc[FIELD_SIZE];
    int  scn_param[FIELD_SIZE];
    char comm[COMM_NAME_SIZE];
	int  cus_lock_hint;
    int  lock_duration;
    int  lock_rsc_size;
    int  *lock_rsc_list;
    int  sports_mode;
    int  priority;
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    int  scn_freq_thermal_ux_max[CLUSTER_MAX];
    //SPD:porting thermal ux policy by sifengtian 20230525 end
}tScnNode;

#endif // ANDROID_PERFSERVICE_H

