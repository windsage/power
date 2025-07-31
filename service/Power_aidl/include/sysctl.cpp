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
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <linux/types.h>
#include <errno.h>
#include <string.h>
#include "sysctl.h"

namespace aidl::android::hardware::power::impl::mediatek {

#define PATH_POWERHAL_IOCTL "/proc/perfmgr_powerhal/ioctl_powerhal_adpf"
#define UNUSED(x) (void)(x)

int powerhalsysdevfd = -1;

static inline int check_powerhal_ioctl_valid(void)
{
    if (powerhalsysdevfd >= 0) {
        return 0;
    } else if (powerhalsysdevfd == -1) {
        powerhalsysdevfd = open(PATH_POWERHAL_IOCTL, O_RDONLY);
        // file not exits
        if (powerhalsysdevfd < 0 && errno == ENOENT) {
            powerhalsysdevfd = -2;
            LOG_E("File not exits");
        }

        // file exist, but can't open
        if (powerhalsysdevfd == -1) {
            LOG_E("Can't open %s: %s", PATH_POWERHAL_IOCTL, strerror(errno));
            return -1;
        }
        // file not exist
    } else if (powerhalsysdevfd == -2) {
        LOG_E("File not exits second");
        return -2;
    }
    return 0;
}

extern "C"
int adpf_sys_get_data(struct _ADPF_PACKAGE *input)
{
    if (check_powerhal_ioctl_valid() == 0) {
        LOG_D("sys ioctl check_powerhal_ioctl_valid");
        ioctl(powerhalsysdevfd, POWERHAL_GET_ADPF_DATA, input);
        return 0;
    }
    LOG_E("sys ioctl check_powerhal_ioctl_valid ERROR");

    return -2;
}

extern "C"
int adpf_sys_set_data(struct _ADPF_PACKAGE *data)
{
    if (check_powerhal_ioctl_valid() == 0) {
        LOG_D("sys ioctl check_powerhal_ioctl_valid");
        ioctl(powerhalsysdevfd, POWERHAL_SET_ADPF_DATA, data);
        return 0;
    }
    LOG_E("sys ioctl check_powerhal_ioctl_valid ERROR");

    return -2;
}

}
