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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <dlfcn.h>
#include <string.h>

#include "perfservice.h"
#include "utility_peak_power.h"
#include "common.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


using namespace std;

#define THERMAL_CORE_SOCKET_NAME    "/dev/socket/thermal_socket"
#define BUFFER_SIZE                 (64)
#define MPMM_ENABLE                 "0xE1E2E3E4"
#define MPMM_DISABLE                "0xF1F2F3F4"

int set_peak_power_budget_mode(int value, void *scn)
{
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", value, scn);
    if (snprintf(str, sizeof(str)-1, "mode %d", value) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    set_value(PATH_PEAK_POWER_MODE, str);

    return 0;
}

int set_cg_min_power(int value, void *scn)
{
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", value, scn);
    if (snprintf(str, sizeof(str)-1, "cmd %d", value) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    set_value(PATH_PEAK_POWER_CG_MIN_POWER, str);

    return 0;
}

int set_cg_ppt_model_option_favor_multiscene(int value, void *scn)
{
    LOG_I("value:%d", value);

    switch (value) {
    case 0:
        set_value(PATH_CG_PPT_MODEL_OPTION, "favor_multiscene 0");
        break;
    case 1:
        set_value(PATH_CG_PPT_MODEL_OPTION, "favor_multiscene 1");
        break;
    default:
        break;
    }

    return 0;
}

int unset_cg_ppt_model_option_favor_multiscene(int value, void *scn)
{
    return 0;
}

static int send_by_socket_thermal_core(char *cmd, int sz)
{
	struct sockaddr_un addr;
	int client_fd, ret;

	client_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (client_fd == -1) {
		LOG_E("Failed to create a local socket\n");
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, THERMAL_CORE_SOCKET_NAME, sizeof(addr.sun_path) - 1);

	ret = connect(client_fd, (const struct sockaddr *) &addr,
			sizeof(struct sockaddr_un));
	if (ret == -1) {
		LOG_E("Server not found\n");
		close(client_fd);
		return -1;
	}

	ret = send(client_fd, cmd, sz,0);
	if (ret == -1)
		LOG_E("Socket send failed: %s\n", strerror(errno));

	close(client_fd);
	return 0;
}

int limit_backlight_power(int reduce_b, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", reduce_b, scn);

    if (snprintf(str, sizeof(str)-1, "backlight_limit %d", reduce_b) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_flashlight_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "flashlight_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_wifi_power(int level, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", level, scn);

    if (snprintf(str, sizeof(str)-1, "wifi_limit %d", level) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int set_md_autonomous_ctrl(int on, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", on, scn);

    if (snprintf(str, sizeof(str)-1, "md_autonomous_ctrl %d", on) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_ul_throttle_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_ul_throttle_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_txpower_lte_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_txpower_lte_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_cc_ctrl_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_cc_ctrl_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int unlimit_md_cc_ctrl_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    // 16777215(0x00_FF_FF_FF) means no throttle
    if (snprintf(str, sizeof(str)-1, "md_cc_ctrl_limit 16777215") < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_headswitch_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_headswitch_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_ras_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_ras_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_txpower_fr2_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_txpower_fr2_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_flightmode_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_flightmode_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_charger_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_charger_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_txpower_fr1_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_txpower_fr1_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_uai_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_uai_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int unlimit_md_uai_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    // 4294967295(0xFF_FF_FF_FF) means no throttle
    if (snprintf(str, sizeof(str)-1, "md_uai_limit 4294967295") < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int limit_md_sw_shutdown_power(int limit_state, void *scn)
{
    int ret = 0;
    char str[BUFFER_SIZE];

    LOG_D("%d, scn:%p", limit_state, scn);

    if (snprintf(str, sizeof(str)-1, "md_sw_shutdown_limit %d", limit_state) < 0) {
        LOG_E("snprintf error");
        return -1;
    }
    ret = send_by_socket_thermal_core(str, sizeof(str));

    return ret;
}

int set_mpmm_enable(int enable, void *scn)
{
    char str[64];

    LOG_D("%d, scn:%p", enable, scn);

    if (enable)
        set_value(PATH_MCUPM_MPMM, MPMM_DISABLE);
    else
        set_value(PATH_MCUPM_MPMM, MPMM_ENABLE);

    return 0;
}

int set_pt_enable(int enable, void *scn)
{
    char str[64];

    ALOGD("set_pt_enable: enable:%d, scn:%p", enable, scn);

    if (enable) {
        set_value(PATH_PT_LB_STOP, "stop 0");
        set_value(PATH_PT_OC_STOP, "stop 0");
    } else {
        set_value(PATH_PT_LB_UT, "Utest 0");
        set_value(PATH_PT_OC_UT, "Utest 0");
        set_value(PATH_PT_LB_STOP, "stop 1");
        set_value(PATH_PT_OC_STOP, "stop 1");
    }

    return 0;
}
