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

#ifndef ANDROID_UTILITY_SYS_H
#define ANDROID_UTILITY_SYS_H

#define PATH_TASK_PREFER_CPU          "/sys/devices/system/cpu/sched/cpu_prefer"
#define PATH_EAS_CTRL_TASK_PREFER_CPU "/proc/perfmgr/boost_ctrl/eas_ctrl/cpu_prefer"
#define PATH_CPU_CTRL_ISO_CPU         "/proc/perfmgr/boost_ctrl/cpu_ctrl/perfserv_iso_cpu"
#define PATH_TASK_TURBO_ENFORCE_CT_ENBALE  "/sys/module/vip_engine/parameters/enforce_ct_to_vip_param"
#define PATH_TASK_TURBO_SET_PER_TASK_VIP_ENABLE  "/sys/module/vip_engine/parameters/enable_tgd"
#define PATH_TASK_TURBO_SET_PER_TASK_VIP  "/sys/module/vip_engine/parameters/set_tgd"
#define PATH_TASK_TURBO_UNSET_PER_TASK_VIP  "/sys/module/vip_engine/parameters/unset_tgd"

extern int initTaskPreferCpu(int power_on);
extern int setTaskPreferCpu_big(int tid, void *scn);
extern int unsetTaskPreferCpu_big(int tid, void *scn);
extern int setTaskPreferCpu_little(int tid, void *scn);
extern int unsetTaskPreferCpu_little(int tid, void *scn);
extern int init_isolation_cpu(int power_on);
extern int set_isolation_cpu(int tid, void *scn);
extern int unset_isolation_cpu(int tid, void *scn);
extern int set_cpuidle(int value, void *scn);
extern int init_cpuidle(int power_on);
extern int set_cus_hint_enable(int enable, void *scn);

extern int set_test_val(int value, void *scn);
extern int set_webp_use_threads_mode(int value, void *scn);
extern int dsu_cci_sport_mode(int value, void *scn);
extern int set_task_turbo_enforce_ct_enable(int value, void *scn);
extern int unset_task_turbo_enforce_ct_enable(int value, void *scn);
extern int set_task_turbo_per_task_vip_enable(int value, void *scn);
extern int set_task_turbo_per_task_vip(int value, void *scn);
extern int unset_task_turbo_per_task_vip(int value, void *scn);
extern int setCPUFreqMinCluster0(int value, void *scn);
extern int setCPUFreqMaxCluster0(int value, void *scn);
extern int setCPUFreqMinCluster1(int value, void *scn);
extern int setCPUFreqMaxCluster1(int value, void *scn);
extern int setCPUFreqMinCluster2(int value, void *scn);
extern int setCPUFreqMaxCluster2(int value, void *scn);
extern int setCPUFreqHardMinCluster0(int value, void *scn);
extern int setCPUFreqHardMaxCluster0(int value, void *scn);
extern int setCPUFreqHardMinCluster1(int value, void *scn);
extern int setCPUFreqHardMaxCluster1(int value, void *scn);
extern int setCPUFreqHardMinCluster2(int value, void *scn);
extern int setCPUFreqHardMaxCluster2(int value, void *scn);
extern int setCPUFreqMinCore0(int value, void *scn);
extern int setCPUFreqMaxCore0(int value, void *scn);
extern int setCPUFreqMinCore1(int value, void *scn);
extern int setCPUFreqMaxCore1(int value, void *scn);
extern int setCPUFreqMinCore2(int value, void *scn);
extern int setCPUFreqMaxCore2(int value, void *scn);
extern int setCPUFreqMinCore3(int value, void *scn);
extern int setCPUFreqMaxCore3(int value, void *scn);
extern int setCPUFreqMinCore4(int value, void *scn);
extern int setCPUFreqMaxCore4(int value, void *scn);
extern int setCPUFreqMinCore5(int value, void *scn);
extern int setCPUFreqMaxCore5(int value, void *scn);
extern int setCPUFreqMinCore6(int value, void *scn);
extern int setCPUFreqMaxCore6(int value, void *scn);
extern int setCPUFreqMinCore7(int value, void *scn);
extern int setCPUFreqMaxCore7(int value, void *scn);
extern int setCPUCoreMinCluster0(int value, void *scn);
extern int setCPUCoreMaxCluster0(int value, void *scn);
extern int setCPUCoreMinCluster1(int value, void *scn);
extern int setCPUCoreMaxCluster1(int value, void *scn);
extern int setCPUCoreMinCluster2(int value, void *scn);
extern int setCPUCoreMaxCluster2(int value, void *scn);

struct _POWERHAL_PACKAGE {
	__u32 value;
};

#define DSU_CCI_SPORT_MODE                _IOW('g', 3, _POWERHAL_PACKAGE)

#endif // ANDROID_UTILITY_FPS_H

