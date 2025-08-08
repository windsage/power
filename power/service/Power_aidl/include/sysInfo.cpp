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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#include <sys/types.h>
#include <linux/types.h>
#include <log/log.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <errno.h>
#include <string.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <dlfcn.h>
#include <aidl/android/hardware/health/IHealth.h>
#include <dlfcn.h>
#include <android/binder_manager.h>
#include "sysctl.h"

namespace aidl::android::hardware::power::impl::mediatek {


#define DRM_DISPLAY_PATH "/dev/dri/card0"
#define ADVICE_BAT_MAX_CURRENT_PATH "/sys/module/mtk_perf_ioctl_magt/parameters/advice_bat_max_current"
#define STRING_BUFFER_SIZE 128

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)


std::shared_ptr<aidl::android::hardware::health::IHealth> g_health_aidl = nullptr;

int drmFd = -1;
int maxDisplayRefreshRate = -1;
struct _drmModeModeInfo * modeInfo = 0;
int IHealthService_supported = 0;
bool GedInitialized = false;


typedef enum GED_INFO_TAG {
    GED_LOADING = 0,
    GED_CUR_FREQ = 5,
    GED_MAX_FREQ = 10,
} GED_INFO;


struct GPUINFO {
    int loading;
    int freq_cur;
    int freq_max;
    int freq_pred;
    int target_fps;
    int frame_time;
};

void *g_hGed;
typedef void *GED_HANDLE;
typedef GED_HANDLE (*create)(void);
typedef void (*destroy)(GED_HANDLE);
typedef int (*query_info)(GED_HANDLE, GED_INFO, size_t, void *);
typedef int (*query_gpu_dvfs_info)(GED_HANDLE, int, int, int *, int *, int *, int *, int *);

create ged_create;
destroy ged_destroy;
query_info ged_query_info;
query_gpu_dvfs_info ged_query_gpu_dvfs_info;



void ensureGedInitialized()
{
    if (GedInitialized)
        return;

    void* func;
    void* ged_lib = dlopen("libged.so", RTLD_NOW | RTLD_LOCAL);

    if (ged_lib == nullptr) {
        LOG_E("Failed to open libged.so! (error=%s)", dlerror());
        return;
    }

    func = dlsym(ged_lib, "ged_create");
    ged_create = reinterpret_cast<create>(func);
    if (!ged_create)
        LOG_E("Failed to find required symbol ged_create!");

    func = dlsym(ged_lib, "ged_destroy");
    ged_destroy = reinterpret_cast<destroy>(func);
    if (!ged_destroy)
        LOG_E("Failed to find required symbol ged_destroy!");

    func = dlsym(ged_lib, "ged_query_info");
    ged_query_info = reinterpret_cast<query_info>(func);
    if (!ged_query_info)
        LOG_E("Failed to find required symbol ged_query_info!");

    func = dlsym(ged_lib, "ged_query_gpu_dvfs_info");
    ged_query_gpu_dvfs_info = reinterpret_cast<query_gpu_dvfs_info>(func);
    if (!ged_query_gpu_dvfs_info)
        LOG_E("Failed to find required symbol ged_query_gpu_dvfs_info!");


    if (g_hGed == NULL) {
        g_hGed = ged_create();

        if (g_hGed == NULL) {
            LOG_E("ged_create() failed!");
            return;
        }
    }

    GedInitialized = true;
    LOG_I("ged_create() successfully!");
}

extern "C"
int get_max_refresh_rate()
{
    bool find = false;

    drmFd = open(DRM_DISPLAY_PATH, O_RDWR);
    if(drmFd < 0) {
        LOG_E("open drm device[%s]: %d", DRM_DISPLAY_PATH, drmFd);
        return drmFd;
    }
    drmModeResPtr res = drmModeGetResources(drmFd);
    drmModePlaneResPtr pres = drmModeGetPlaneResources(drmFd);

    if(res == NULL || pres == NULL) {
        LOG_E("drmModeGetResources or drmModeGetPlaneResources failed.");
        return -1;
    }

    for(size_t i = 0, mask = 1; i < res->count_crtcs && !find; i++, mask = mask << 1) {
        drmModeCrtcPtr crtc = drmModeGetCrtc(drmFd, res->crtcs[i]);

        for(size_t j = 0; j < res->count_encoders && !find; j++) {
            drmModeEncoderPtr encoder = drmModeGetEncoder(drmFd, res->encoders[j]);

            if(encoder == NULL)
                continue;

            if(encoder->possible_crtcs & mask) {
                for(size_t k = 0; k < res->count_connectors && !find; k++) {
                    drmModeConnectorPtr connector = drmModeGetConnector(drmFd, res->connectors[k]);
                    if(connector == NULL)
                        continue;

                    if(connector->connector_type == DRM_MODE_CONNECTOR_DSI) {
                        for(size_t n = 0; n < connector->count_encoders && !find; n++) {
                            if(connector->encoders[n] == encoder->encoder_id) {
                                modeInfo = connector->count_modes > 0 ? &connector->modes[0] : NULL;
                                for(size_t m = 0; m < connector->count_modes; m++) {
                                    modeInfo = &connector->modes[m];
                                    if(modeInfo != NULL) {
                                        LOG_I("possible refresh rate: %d", modeInfo->vrefresh);
                                        int tempRefreshRate = modeInfo->vrefresh;
                                        maxDisplayRefreshRate = (tempRefreshRate > maxDisplayRefreshRate) ? tempRefreshRate : maxDisplayRefreshRate;
                                        LOG_I("modeInfo->vrefresh: %d, maxDisplayRefreshRate: %d", tempRefreshRate, maxDisplayRefreshRate);
                                    }
                                }
                                find = true;
                            }
                        }
                    }
                    drmModeFreeConnector(connector);
                }
            }
            drmModeFreeEncoder(encoder);
        }
        drmModeFreeCrtc(crtc);
    }
    LOG_I("max refresh rate: %d", maxDisplayRefreshRate);

    drmModeFreePlaneResources(pres);
    drmModeFreeResources(res);

    close(drmFd);

    return maxDisplayRefreshRate;
}

int get_cpu_headroom()
{
    _ADPF_PACKAGE msg;
    msg.cmd = GET_CPU_HEADROOM;
    msg.cpuHeadroomResult = -1;

    adpf_sys_get_data(&msg);

    LOG_D("cpuHeadroomResult=%d", msg.cpuHeadroomResult);

    if (msg.cpuHeadroomResult < 0 || msg.cpuHeadroomResult > 100) {
        LOG_E("invalid headroom: %d", msg.cpuHeadroomResult);
        return -1;
    }

    return msg.cpuHeadroomResult;
}

int get_gpu_perf_index()
{
    static GPUINFO gpuinfo;

    ensureGedInitialized();

    if (!ged_query_info) {
        LOG_E("failed to find required symbol ged_query_info");
        return -1;
    }

    int ret = ged_query_gpu_dvfs_info(g_hGed, (int)getpid(), 0, &gpuinfo.freq_cur, &gpuinfo.freq_max, &gpuinfo.freq_pred, &gpuinfo.target_fps, &gpuinfo.frame_time);
    if (ret != 0) {
        LOG_E("failed to call ged_query_gpu_dvfs_info (ret=%d)", ret);
        return -1;
    }

    if (gpuinfo.freq_cur < 0 || gpuinfo.freq_max <= 0) {
        LOG_E("invalid gpu freq value: cur=%d, max=%d", gpuinfo.freq_cur, gpuinfo.freq_max);
        return -1;
    }

    int gpu_perf_idx = (gpuinfo.freq_cur * 100) / gpuinfo.freq_max;
    if (gpu_perf_idx > 100)
        gpu_perf_idx = 100;
    LOG_D("gpu_perf_idx=%d, freq_cur=%d, freq_max=%d", gpu_perf_idx, gpuinfo.freq_cur, gpuinfo.freq_max);
    return gpu_perf_idx;
}



}