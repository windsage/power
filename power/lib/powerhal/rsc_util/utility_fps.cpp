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
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "common.h"
#include "tran_common.h"
#include "utility_fps.h"

#define FSTB_FPS_LEN    32

#define PATH_MAX_FPS_PID       "/sys/kernel/fpsgo/fstb/set_render_max_fps"
#define PATH_CTRL_RENDER     "/sys/kernel/fpsgo/composer/set_ui_ctrl"
#define PATH_SET_VIDEO_PID     "/sys/kernel/fpsgo/fstb/set_video_pid"
#define PATH_CLEAR_VIDEO_PID     "/sys/kernel/fpsgo/fstb/clear_video_pid"
#define PATH_BYPASS_NON_SF      "/sys/kernel/fpsgo/composer/bypass_non_SF_by_pid"
#define PATH_CONTROL_API_MASK   "/sys/kernel/fpsgo/composer/control_api_mask_by_pid"
#define PATH_CONTROL_HWUI       "/sys/kernel/fpsgo/composer/control_hwui_by_pid"
#define PATH_CAM_FPS_MARGIN  "/sys/kernel/fpsgo/composer/app_cam_fps_align_margin_by_pid"
#define PATH_CAM_TIME_RATIO  "/sys/kernel/fpsgo/composer/app_cam_time_align_ratio_by_pid"
#define PATH_JANK_DETECTION_MASK  "/sys/kernel/jank_detection/jank_detection"
#define PATH_DEP_LOADING_THR  "/sys/kernel/fpsgo/composer/dep_loading_thr_by_pid"
#define PATH_CAM_WINDOW_MS    "/sys/kernel/fpsgo/composer/cam_bypass_window_ms_by_pid"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

#define FBT_SET_ATTR_BY_PID(name) \
int set_##name##_by_pid(int value, void *scn) { \
    return write_cmd_to_file(render_pid, 's', value, PATH_FBT_ATTR_PARAMS_BY_PID, #name); \
}

#define FBT_UNSET_ATTR_BY_PID(name) \
int unset_##name##_by_pid(int value, void *scn) { \
    return write_cmd_to_file(render_pid, 'u', -1, PATH_FBT_ATTR_PARAMS_BY_PID, #name); \
}

#define FBT_SET_ATTR_BY_TID(name) \
int set_##name##_by_tid(int value, void *scn) { \
    return write_cmd_to_file(render_tid, 's', value, PATH_FBT_ATTR_PARAMS_BY_TID, #name); \
}

#define FBT_UNSET_ATTR_BY_TID(name) \
int unset_##name##_by_tid(int value, void *scn) { \
    return write_cmd_to_file(render_tid, 'u', -1, PATH_FBT_ATTR_PARAMS_BY_TID, #name); \
}


//using namespace std;

/* variable */
static int fstb_support = 0;
static int render_pid = -1;
static int render_tid;
static int render_bufID_high_bits;
static int render_bufID_low_bits;
#if defined(__FSTB_FPS__) // Not defined due to PATH_FSTB_FPS is phased out
static char pFstbDefaultFps[FSTB_FPS_LEN];
static int fps_high_now = -1;
static int fps_low_now = -1;
#endif
static char pFstbDefaultSoftFps[FSTB_FPS_LEN];
static int fps_soft_high_now = -1;
static int fps_soft_low_now = -1;
static int tran_powerhal_encode = 1;

/************************
 * function
 ************************/

static void init_fstb(void)
{
    char line[256];
    FILE* file = NULL;
    struct stat stat_buf;
    time_t data_mtime = 0, system_mtime = 0;

    if (0 == stat(PATH_FSTB_LIST_FILE_2, &stat_buf))
        data_mtime = stat_buf.st_mtime;

    if (0 == stat(PATH_FSTB_LIST_FILE, &stat_buf))
        system_mtime = stat_buf.st_mtime;


    //SPD: add xml decode by fan.feng1 20220922 start
    if(tran_powerhal_encode) {
        char *buf_decode = NULL;
        if(data_mtime > system_mtime) {
            buf_decode = get_decode_buf(PATH_FSTB_LIST_FILE_2);
        } else {
            buf_decode = get_decode_buf(PATH_FSTB_LIST_FILE);
        }

        char *token = strtok(buf_decode, "\n");
        while(token) {
            set_value(PATH_FSTB_LIST, token);
            token = strtok(NULL, "\n");
        }
        free(buf_decode);
    } else {
        file = fopen(PATH_FSTB_LIST_FILE, "r");
        if (file) {
            while (fgets(line, sizeof(line), file)) {
                set_value(PATH_FSTB_LIST, line);
                LOG_D("list1 %s", line);
            }
            if(fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
        }

        if (data_mtime < system_mtime)
            return;

        file = fopen(PATH_FSTB_LIST_FILE_2, "r");
        if (file) {
            while (fgets(line, sizeof(line), file)) {
                set_value(PATH_FSTB_LIST, line);
                LOG_D("list2 %s", line);
            }
            if(fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
        }
    }
    //SPD: add xml decode by fan.feng1 20220922 end
}


static void init_gbe(void)
{
    /*char file_char_array[1024];*/
    char line[256];
    //SPD: add xml decode by fan.feng1 20220922 start
    if(tran_powerhal_encode) {
        char *buf_decode = NULL;
        struct stat stat_buf;

        if(0 == stat(PATH_GBE_LIST_FILE, &stat_buf)) {
            buf_decode = get_decode_buf(PATH_GBE_LIST_FILE);
            char *token = strtok(buf_decode, "\n");
            while(token) {
                set_value(PATH_GBE_LIST, token);
                token = strtok(NULL, "\n");
            }
            free(buf_decode);
        }
    } else {
        FILE* file = fopen(PATH_GBE_LIST_FILE, "r");

        if (file) {
            while (fgets(line, sizeof(line), file)) {
                set_value(PATH_GBE_LIST, line);
                LOG_I("%s", line);
            }
            if(fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
        }
    }
    //SPD: add xml decode by fan.feng1 20220922 end
}

static void init_xgf(void)
{
    /*char file_char_array[1024];*/
    char line[256];
    //SPD: add xml decode by fan.feng1 20220922 start
    if(tran_powerhal_encode) {
        char *buf_decode = NULL;
        struct stat stat_buf;

        if(0 == stat(PATH_XGF_LIST_FILE, &stat_buf)) {
            buf_decode = get_decode_buf(PATH_XGF_LIST_FILE);
            char *token = strtok(buf_decode, "\n");
            while(token) {
                set_value(PATH_XGF_LIST, token);
                token = strtok(NULL, "\n");
            }
            free(buf_decode);
        }
    } else {
        FILE* file = fopen(PATH_XGF_LIST_FILE, "r");

        if (file) {
            while (fgets(line, sizeof(line), file)) {
                set_value(PATH_XGF_LIST, line);
                LOG_I("%s", line);
            }
            if(fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
        }
    }
    //SPD: add xml decode by fan.feng1 20220922 end
}

#if defined(__FSTB_FPS__)
static void setFstbFps(int fps_high, int fps_low)
{
    char fstb_fps[FSTB_FPS_LEN] = "";
    if (fstb_support) {
        if (fps_high == -1 && fps_low == -1) {
            LOG_V("set fstb_fps: %s", pFstbDefaultFps);
            set_value(PATH_FSTB_FPS, pFstbDefaultFps);
        } else {
            if(snprintf(fstb_fps, sizeof(fstb_fps), "1 %d-%d", fps_high_now, fps_low_now) < 0)
                return;
            LOG_V("set fstb_fps: %s", fstb_fps);
            set_value(PATH_FSTB_FPS, fstb_fps);
        }
   }
}
#endif

static void setFstbSoftFps(int fps_high, int fps_low)
{
    char fstb_fps[FSTB_FPS_LEN] = "";
    if (fstb_support) {
        if (fps_high == -1 && fps_low == -1) {
            LOG_V("set fstb_fps: %s", pFstbDefaultSoftFps);
            set_value(PATH_FSTB_SOFT_FPS, pFstbDefaultSoftFps);
        } else {
            if(snprintf(fstb_fps, sizeof(fstb_fps), "1 %d-%d", fps_high, fps_low) < 0)
                return;
            LOG_V("set fstb_fps: %s", fstb_fps);
            set_value(PATH_FSTB_SOFT_FPS, fstb_fps);
        }
   }
}

/* function */
int fstb_init(int power_on)
{
    struct stat stat_buf;
    char str2[FSTB_FPS_LEN];

    LOG_V("%d", power_on);
    fstb_support = (0 == stat(PATH_FSTB_STATUS, &stat_buf)) ? 1 : 0;

    if(fstb_support) {
#if defined(__FSTB_FPS__)
        get_str_value(PATH_FSTB_FPS, str2, sizeof(str2)-1);
        strncpy(pFstbDefaultFps, str2, FSTB_FPS_LEN - 1);
        LOG_I("pFstbDefaultFps:%s", pFstbDefaultFps);
#endif
        get_str_value(PATH_FSTB_SOFT_FPS, str2, sizeof(str2)-1);
        strncpy(pFstbDefaultSoftFps, str2, FSTB_FPS_LEN - 1);
        if (0 == stat(PATH_FSTB_LIST, &stat_buf))
            init_fstb();
        if (0 == stat(PATH_GBE_LIST, &stat_buf))
            init_gbe();
        if (0 == stat(PATH_XGF_LIST, &stat_buf))
            init_xgf();
    }
    return 0;
}

/*
 * user should not use loom.cfg and loom_init()
 * please use loom.cfg.xml
 */
int loom_init(int power_on_init)
{
    /*char file_char_array[1024];*/
    char line[256];
    struct stat stat_buf;
    FILE* file = NULL;

    LOG_V("%d", power_on_init);
    if (0 == stat(PATH_LOOM_TASK_CFG, &stat_buf)) {
        file = fopen(PATH_LOOM_TASK_CFG_FILE, "r");

        if (file) {
            while (fgets(line, sizeof(line), file)) {
                set_value(PATH_LOOM_TASK_CFG, line);
                LOG_I("%s", line);
            }
            if (fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
        }
    }
    return 0;
}

#if defined(__FSTB_FPS__)
int setFstbFpsHigh(int fps_high, void *scn)
{
    LOG_V("setFstbFpsHigh: %p", scn);
    fps_high_now = fps_high;
    setFstbFps(fps_high_now, fps_low_now);
    return 0;
}

int setFstbFpsLow(int fps_low, void *scn)
{
    LOG_V("setFstbFpsLow: %p", scn);
    fps_low_now = fps_low;
    setFstbFps(fps_high_now, fps_low_now);
    return 0;
}
#endif

int setFstbSoftFpsHigh(int fps_high, void *scn)
{
    LOG_V("setFstbSoftFpsHigh: %p", scn);
    fps_soft_high_now = fps_high;
    setFstbSoftFps(fps_soft_high_now, fps_soft_low_now);
    return 0;
}

int setFstbSoftFpsLow(int fps_low, void *scn)
{
    LOG_V("setFstbSoftFpsLow: %p", scn);
    fps_soft_low_now = fps_low;
    setFstbSoftFps(fps_soft_high_now, fps_soft_low_now);
    return 0;
}

int notify_targetFPS_pid(int pid, void *scn)
{
    LOG_I("%d, %p", pid, scn);

    if (fstb_support)
        set_value(PATH_MAX_FPS_PID, pid);

    return 0;
}

int fpsgo_ctrl_render_thread(int rtid, void *scn)
{
    LOG_I("%d, %p", rtid, scn);

    if (fstb_support) {
        set_value(PATH_CTRL_RENDER, rtid);
    }

    return 0;
}

int fpsgo_set_video_pid_thread(int rtid, void *scn) {
    LOG_I("%d, %p", rtid, scn);

    if (fstb_support)
        set_value(PATH_SET_VIDEO_PID, rtid);

    return 0;
}

int fpsgo_clear_video_pid_thread(int rtid, void *scn) {
    LOG_I("%d, %p", rtid, scn);

    if (fstb_support)
        set_value(PATH_CLEAR_VIDEO_PID, rtid);

    return 0;
}

int write_string_to_file(int pid, int value, char * path) {
    char str[128] = "";

    if (snprintf(str, sizeof(str), "%d %d", pid, value) < 0) {
        LOG_E("sprintf error");
        return -1;
    }
    LOG_I(" %s %s", str, path);
    set_value(path, str);

    return 0;
}

int write_string_to_file_w_BQID(int pid, int high_bits, int low_bits,
    int value, char * path) {
    char str[128] = "";
    unsigned long long buffer_id = (unsigned long long)high_bits;

    buffer_id <<= 32;
    buffer_id |= (unsigned long long)low_bits;

    if (snprintf(str, sizeof(str), "%d %llu %d", pid, buffer_id, value) < 0) {
        LOG_E("sprintf error");
        return -1;
    }
    LOG_I(" %d %d %s %s", high_bits, low_bits, str, path);
    set_value(path, str);

    return 0;
}

int write_cmd_to_file(int pid, char action, int value, char * path, char * cmd) {
    char str[128] = "";

    if (snprintf(str, sizeof(str), "%s %c %d %d", cmd, action, pid, value) < 0) {
        LOG_E("sprintf error");
        return -1;
    }
    LOG_I(" %s %s", str, path);
    set_value(path, str);

    return 0;
}

int set_fpsgo_unsupported_func(int value, void *scn) {
    LOG_I("NOT SUPPORTED! value:%d", value);
    return 0;
}

int unset_fpsgo_unsupported_func(int value, void *scn) {
    LOG_I("NOT SUPPORTED! value:%d", value);
    return 0;
}

int set_fpsgo_render_pid(int value, void *scn) {
    LOG_D("%d", value);
    render_pid = value;

    return 0;
}

int get_fpsgo_render_pid(void) {
    return render_pid;
}

int set_fpsgo_render_tid(int value, void *scn) {
    LOG_D("%d", value);
    render_tid = value;

    return 0;
}

int set_fpsgo_render_bufID_high_bits(int value, void *scn) {
    LOG_D("%d", value);
    render_bufID_high_bits = value;

    return 0;
}

int set_fpsgo_render_bufID_low_bits(int value, void *scn) {
    LOG_D("%d", value);
    render_bufID_low_bits = value;

    return 0;
}

int set_fpsgo_control_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FPSGO_CONTROL_BY_PID);
}

int unset_fpsgo_control_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, 0, PATH_FPSGO_CONTROL_BY_PID);
}

int set_fpsgo_control_by_render(int value, void *scn) {
    return write_string_to_file_w_BQID(render_tid, render_bufID_high_bits,
        render_bufID_low_bits, value, PATH_FPSGO_CONTROL_BY_TID);
}

int unset_fpsgo_control_by_render(int value, void *scn) {
    return write_string_to_file_w_BQID(render_tid, render_bufID_high_bits,
        render_bufID_low_bits, 0, PATH_FPSGO_CONTROL_BY_TID);
}

int set_fpsgo_no_boost_by_process(int value, void *scn) {
    LOG_I("%d", value);
    set_value(PATH_FPSGO_NO_BOOST_BY_PID, value);
    return 0;
}

int set_fpsgo_no_boost_by_thread(int value, void *scn) {
    set_value(PATH_FPSGO_NO_BOOST_BY_TID, value);
    return 0;
}

int set_fpsgo_ema2_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FPSGO_EMA2_ENABLE_BY_PID);
}

int unset_fpsgo_ema2_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_FPSGO_EMA2_ENABLE_BY_PID);
}

int set_fpsgo_calculate_dep_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FPSGO_CALCULATE_DEP_ENABLE_BY_PID);
}

int unset_fpsgo_calculate_dep_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_FPSGO_CALCULATE_DEP_ENABLE_BY_PID);
}

int set_fpsgo_filter_dep_task_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FPSGO_FILTER_DEP_TASK_ENABLE_BY_PID);
}

int unset_fpsgo_filter_dep_task_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_FPSGO_FILTER_DEP_TASK_ENABLE_BY_PID);
}

int set_fstb_target_fps_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FSTB_TARGET_FPS_ENABLE_BY_PID);
}

int unset_fstb_target_fps_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_FSTB_TARGET_FPS_ENABLE_BY_PID);
}

int set_fstb_target_detect_enable_by_render(int value, void *scn) {
    return write_string_to_file_w_BQID(render_tid, render_bufID_high_bits,
        render_bufID_low_bits, value, PATH_FSTB_TARGET_DETECT_ENABLE_BY_TID);
}

int unset_fstb_target_detect_enable_by_render(int value, void *scn) {
    return write_string_to_file_w_BQID(render_tid, render_bufID_high_bits,
        render_bufID_low_bits, -1, PATH_FSTB_TARGET_DETECT_ENABLE_BY_TID);
}

int set_notify_fps_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FSTB_NOTIFY_FPS_BY_PID);
}

int unset_notify_fps_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, 0, PATH_FSTB_NOTIFY_FPS_BY_PID);
}

int set_mfrc_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_GAME_FI_ENABLE_BY_PID);
}

int unset_mfrc_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, 0, PATH_GAME_FI_ENABLE_BY_PID);
}

int set_loom_enable_by_pid(int value, void *scn) {
    LOG_I("%d", value);
    return write_string_to_file(render_pid, value, PATH_LOOM_ENABLE_BY_PID);
}

int unset_loom_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, 0, PATH_LOOM_ENABLE_BY_PID);
}

int set_game_target_fps_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_GAME_SET_TARGET_FPS_BY_PID);
}
int unset_game_target_fps_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_GAME_SET_TARGET_FPS_BY_PID);
}

int set_bypass_nonsf_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_BYPASS_NON_SF);
}

int unset_bypass_nonsf_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_BYPASS_NON_SF);
}

int set_control_api_mask_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_CONTROL_API_MASK);
}

int unset_control_api_mask_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_CONTROL_API_MASK);
}

int set_control_hwui_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_CONTROL_HWUI);
}

int unset_control_hwui_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_CONTROL_HWUI);
}

int set_app_cam_fps_align_margin_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_CAM_FPS_MARGIN);
}

int unset_app_cam_fps_align_margin_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_CAM_FPS_MARGIN);
}

int set_app_cam_time_align_ratio_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_CAM_TIME_RATIO);
}

int unset_app_cam_time_align_ratio_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_CAM_TIME_RATIO);
}

int set_jank_detection_mask(int value, void *scn) {
    set_value(PATH_JANK_DETECTION_MASK, value);
    return 0;
}

int unset_jank_detection_mask(int value, void *scn) {
    if (value == 17) {
        value = 1;
    }
    set_value(PATH_JANK_DETECTION_MASK, value);
    return 0;
}

int set_dep_loading_thr_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_DEP_LOADING_THR);
}

int unset_dep_loading_thr_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_DEP_LOADING_THR);
}

int set_cam_bypass_window_ms_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_CAM_WINDOW_MS);
}

int unset_cam_bypass_window_ms_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, -1, PATH_CAM_WINDOW_MS);
}

int set_fstbv2_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, value, PATH_FSTBV2_ENABLE_BY_PID);
}

int unset_fstbv2_enable_by_pid(int value, void *scn) {
    return write_string_to_file(render_pid, 0, PATH_FSTBV2_ENABLE_BY_PID);
}

FBT_SET_ATTR_BY_PID(rescue_enable)
FBT_UNSET_ATTR_BY_PID(rescue_enable)

FBT_SET_ATTR_BY_PID(rescue_second_enable)
FBT_UNSET_ATTR_BY_PID(rescue_second_enable)

FBT_SET_ATTR_BY_PID(loading_th)
FBT_UNSET_ATTR_BY_PID(loading_th)

FBT_SET_ATTR_BY_PID(light_loading_policy)
FBT_UNSET_ATTR_BY_PID(light_loading_policy)

FBT_SET_ATTR_BY_PID(llf_task_policy)
FBT_UNSET_ATTR_BY_PID(llf_task_policy)

FBT_SET_ATTR_BY_PID(filter_frame_enable)
FBT_UNSET_ATTR_BY_PID(filter_frame_enable)

FBT_SET_ATTR_BY_PID(separate_aa)
FBT_UNSET_ATTR_BY_PID(separate_aa)

FBT_SET_ATTR_BY_PID(separate_release_sec)
FBT_UNSET_ATTR_BY_PID(separate_release_sec)

FBT_SET_ATTR_BY_PID(boost_affinity)
FBT_UNSET_ATTR_BY_PID(boost_affinity)

FBT_SET_ATTR_BY_PID(boost_lr)
FBT_UNSET_ATTR_BY_PID(boost_lr)

FBT_SET_ATTR_BY_PID(cpumask_heavy)
FBT_UNSET_ATTR_BY_PID(cpumask_heavy)

FBT_SET_ATTR_BY_PID(cpumask_second)
FBT_UNSET_ATTR_BY_PID(cpumask_second)

FBT_SET_ATTR_BY_PID(cpumask_others)
FBT_UNSET_ATTR_BY_PID(cpumask_others)

FBT_SET_ATTR_BY_PID(set_soft_affinity)
FBT_UNSET_ATTR_BY_PID(set_soft_affinity)

FBT_SET_ATTR_BY_PID(sep_loading_ctrl)
FBT_UNSET_ATTR_BY_PID(sep_loading_ctrl)

FBT_SET_ATTR_BY_PID(lc_th)
FBT_UNSET_ATTR_BY_PID(lc_th)

FBT_SET_ATTR_BY_PID(lc_th_upbound)
FBT_UNSET_ATTR_BY_PID(lc_th_upbound)

FBT_SET_ATTR_BY_PID(frame_lowbd)
FBT_UNSET_ATTR_BY_PID(frame_lowbd)

FBT_SET_ATTR_BY_PID(frame_upbd)
FBT_UNSET_ATTR_BY_PID(frame_upbd)

FBT_SET_ATTR_BY_PID(limit_uclamp)
FBT_UNSET_ATTR_BY_PID(limit_uclamp)

FBT_SET_ATTR_BY_PID(limit_ruclamp)
FBT_UNSET_ATTR_BY_PID(limit_ruclamp)

FBT_SET_ATTR_BY_PID(limit_uclamp_m)
FBT_UNSET_ATTR_BY_PID(limit_uclamp_m)

FBT_SET_ATTR_BY_PID(limit_ruclamp_m)
FBT_UNSET_ATTR_BY_PID(limit_ruclamp_m)

FBT_SET_ATTR_BY_PID(qr_enable)
FBT_UNSET_ATTR_BY_PID(qr_enable)

FBT_SET_ATTR_BY_PID(qr_t2wnt_x)
FBT_UNSET_ATTR_BY_PID(qr_t2wnt_x)

FBT_SET_ATTR_BY_PID(qr_t2wnt_y_p)
FBT_UNSET_ATTR_BY_PID(qr_t2wnt_y_p)

FBT_SET_ATTR_BY_PID(qr_t2wnt_y_n)
FBT_UNSET_ATTR_BY_PID(qr_t2wnt_y_n)

FBT_SET_ATTR_BY_PID(gcc_enable)
FBT_UNSET_ATTR_BY_PID(gcc_enable)

FBT_SET_ATTR_BY_PID(gcc_fps_margin)
FBT_UNSET_ATTR_BY_PID(gcc_fps_margin)

FBT_SET_ATTR_BY_PID(gcc_up_sec_pct)
FBT_UNSET_ATTR_BY_PID(gcc_up_sec_pct)

FBT_SET_ATTR_BY_PID(gcc_down_sec_pct)
FBT_UNSET_ATTR_BY_PID(gcc_down_sec_pct)

FBT_SET_ATTR_BY_PID(gcc_down_step)
FBT_UNSET_ATTR_BY_PID(gcc_down_step)

FBT_SET_ATTR_BY_PID(gcc_reserved_up_quota_pct)
FBT_UNSET_ATTR_BY_PID(gcc_reserved_up_quota_pct)

FBT_SET_ATTR_BY_PID(gcc_up_step)
FBT_UNSET_ATTR_BY_PID(gcc_up_step)

FBT_SET_ATTR_BY_PID(gcc_reserved_down_quota_pct)
FBT_UNSET_ATTR_BY_PID(gcc_reserved_down_quota_pct)

FBT_SET_ATTR_BY_PID(gcc_enq_bound_thrs)
FBT_UNSET_ATTR_BY_PID(gcc_enq_bound_thrs)

FBT_SET_ATTR_BY_PID(gcc_deq_bound_thrs)
FBT_UNSET_ATTR_BY_PID(gcc_deq_bound_thrs)

FBT_SET_ATTR_BY_PID(gcc_enq_bound_quota)
FBT_UNSET_ATTR_BY_PID(gcc_enq_bound_quota)

FBT_SET_ATTR_BY_PID(gcc_deq_bound_quota)
FBT_UNSET_ATTR_BY_PID(gcc_deq_bound_quota)

FBT_SET_ATTR_BY_PID(separate_pct_m)
FBT_UNSET_ATTR_BY_PID(separate_pct_m)

FBT_SET_ATTR_BY_PID(separate_pct_b)
FBT_UNSET_ATTR_BY_PID(separate_pct_b)

FBT_SET_ATTR_BY_PID(separate_pct_other)
FBT_UNSET_ATTR_BY_PID(separate_pct_other)

FBT_SET_ATTR_BY_PID(blc_boost)
FBT_UNSET_ATTR_BY_PID(blc_boost)

FBT_SET_ATTR_BY_PID(limit_cfreq2cap)
FBT_UNSET_ATTR_BY_PID(limit_cfreq2cap)

FBT_SET_ATTR_BY_PID(limit_rfreq2cap)
FBT_UNSET_ATTR_BY_PID(limit_rfreq2cap)

FBT_SET_ATTR_BY_PID(limit_cfreq2cap_m)
FBT_UNSET_ATTR_BY_PID(limit_cfreq2cap_m)

FBT_SET_ATTR_BY_PID(limit_rfreq2cap_m)
FBT_UNSET_ATTR_BY_PID(limit_rfreq2cap_m)

FBT_SET_ATTR_BY_PID(target_time_up_bound)
FBT_UNSET_ATTR_BY_PID(target_time_up_bound)

FBT_SET_ATTR_BY_PID(boost_VIP)
FBT_UNSET_ATTR_BY_PID(boost_VIP)

FBT_SET_ATTR_BY_PID(group_by_lr)
FBT_UNSET_ATTR_BY_PID(group_by_lr)

FBT_SET_ATTR_BY_PID(heavy_group_num)
FBT_UNSET_ATTR_BY_PID(heavy_group_num)

FBT_SET_ATTR_BY_PID(second_group_num)
FBT_UNSET_ATTR_BY_PID(second_group_num)

FBT_SET_ATTR_BY_PID(set_ls)
FBT_UNSET_ATTR_BY_PID(set_ls)

FBT_SET_ATTR_BY_PID(ls_groupmask)
FBT_UNSET_ATTR_BY_PID(ls_groupmask)

FBT_SET_ATTR_BY_PID(vip_mask)
FBT_UNSET_ATTR_BY_PID(vip_mask)

FBT_SET_ATTR_BY_PID(set_vvip)
FBT_UNSET_ATTR_BY_PID(set_vvip)

FBT_SET_ATTR_BY_PID(vip_throttle)
FBT_UNSET_ATTR_BY_PID(vip_throttle)

FBT_SET_ATTR_BY_PID(set_l3_cache_ct)
FBT_UNSET_ATTR_BY_PID(set_l3_cache_ct)

FBT_SET_ATTR_BY_PID(bm_th)
FBT_UNSET_ATTR_BY_PID(bm_th)

FBT_SET_ATTR_BY_PID(ml_th)
FBT_UNSET_ATTR_BY_PID(ml_th)

FBT_SET_ATTR_BY_PID(tp_policy)
FBT_UNSET_ATTR_BY_PID(tp_policy)

FBT_SET_ATTR_BY_PID(tp_strict_middle)
FBT_UNSET_ATTR_BY_PID(tp_strict_middle)

FBT_SET_ATTR_BY_PID(tp_strict_little)
FBT_UNSET_ATTR_BY_PID(tp_strict_little)

FBT_SET_ATTR_BY_PID(gh_prefer)
FBT_UNSET_ATTR_BY_PID(gh_prefer)

FBT_SET_ATTR_BY_PID(reset_taskmask)
FBT_UNSET_ATTR_BY_PID(reset_taskmask)

FBT_SET_ATTR_BY_PID(check_buffer_quota)
FBT_UNSET_ATTR_BY_PID(check_buffer_quota)

FBT_SET_ATTR_BY_PID(expected_fps_margin)
FBT_UNSET_ATTR_BY_PID(expected_fps_margin)

FBT_SET_ATTR_BY_PID(aa_b_minus_idle_t)
FBT_UNSET_ATTR_BY_PID(aa_b_minus_idle_t)

FBT_SET_ATTR_BY_PID(quota_v2_diff_clamp_min)
FBT_UNSET_ATTR_BY_PID(quota_v2_diff_clamp_min)

FBT_SET_ATTR_BY_PID(quota_v2_diff_clamp_max)
FBT_UNSET_ATTR_BY_PID(quota_v2_diff_clamp_max)

FBT_SET_ATTR_BY_PID(rl_l2q_enable)
FBT_UNSET_ATTR_BY_PID(rl_l2q_enable)

FBT_SET_ATTR_BY_PID(rl_l2q_exp_us)
FBT_UNSET_ATTR_BY_PID(rl_l2q_exp_us)

FBT_SET_ATTR_BY_PID(powerRL_enable)
FBT_UNSET_ATTR_BY_PID(powerRL_enable)

FBT_SET_ATTR_BY_PID(powerRL_FPS_margin)
FBT_UNSET_ATTR_BY_PID(powerRL_FPS_margin)

FBT_SET_ATTR_BY_PID(powerRL_cap_limit_range)
FBT_UNSET_ATTR_BY_PID(powerRL_cap_limit_range)

FBT_SET_ATTR_BY_PID(aa_enable)
FBT_UNSET_ATTR_BY_PID(aa_enable)

FBT_SET_ATTR_BY_TID(aa_enable)
FBT_UNSET_ATTR_BY_TID(aa_enable)
