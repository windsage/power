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

#define DRM_DISPLAY_PATH "/dev/dri/card0"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

int drmFd = -1;
int maxDisplayRefreshRate = -1;
struct _drmModeModeInfo * modeInfo = 0;

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
