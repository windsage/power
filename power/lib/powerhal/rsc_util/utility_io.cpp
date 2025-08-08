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
#include <cutils/properties.h>
#include <dirent.h>

#include "common.h"
#include "utility_io.h"
#include "perfservice.h"

#define PATH_BLKDEV_UFS_USER      "/sys/block/sdc/queue/read_ahead_kb"
#define PATH_BLKDEV_DM_USER       "/sys/block/dm-2/queue/read_ahead_kb"
#define PATH_BLKDEV_EMMC_USER     "/sys/block/mmcblk0/queue/read_ahead_kb"

static int blkdev_init = 0;
static int blkdev_ufsSupport = 0;
static int blkdev_dmSupport = 0;
static int blkdev_emmcSupport = 0;
static int f2fs_flushSupport = -1;
static int blkdev_ufsDefault = 0;
static int blkdev_dmDefault = 0;
static int blkdev_emmDefault = 0;
static int f2fs_flushDefault = 0;


/* static function */
static void check_blkDevSupport(void)
{
    struct stat stat_buf;

    if (0 == stat(PATH_BLKDEV_UFS_USER, &stat_buf)) {
        blkdev_ufsSupport = 1;
        blkdev_ufsDefault = get_int_value(PATH_BLKDEV_UFS_USER);
    }
    if (0 == stat(PATH_BLKDEV_DM_USER, &stat_buf)) {
        blkdev_dmSupport = 1;
        blkdev_dmDefault = get_int_value(PATH_BLKDEV_DM_USER);
    }
    if (0 == stat(PATH_BLKDEV_EMMC_USER, &stat_buf)) {
        blkdev_emmcSupport = 1;
        blkdev_emmDefault = get_int_value(PATH_BLKDEV_EMMC_USER);
    }
    ALOGI("check_blkDevSupport: %d, %d, %d", blkdev_ufsDefault, blkdev_dmDefault, blkdev_emmDefault);
}

int setBlkDev_readAhead(int value, void *scn)
{
    ALOGV("setBlkDev_readAhead: %p", scn);
    if (!blkdev_init) {
        check_blkDevSupport();
        blkdev_init = 1;
    }

    if (value != -1) {
        if (blkdev_ufsSupport)
            set_value(PATH_BLKDEV_UFS_USER, value);
        if (blkdev_dmSupport)
            set_value(PATH_BLKDEV_DM_USER, value);
        if (blkdev_emmcSupport)
            set_value(PATH_BLKDEV_EMMC_USER, value);
    } else {
        if (blkdev_ufsSupport)
            set_value(PATH_BLKDEV_UFS_USER, blkdev_ufsDefault);
        if (blkdev_dmSupport)
            set_value(PATH_BLKDEV_DM_USER, blkdev_dmDefault);
        if (blkdev_emmcSupport)
            set_value(PATH_BLKDEV_EMMC_USER, blkdev_emmDefault);
    }
    ALOGI("setBlkDev_readAhead: value:%d", value);
    return 0;
}


#define PROC_MOUNTS_FILENAME  "/proc/mounts"
#define BLOCK_PATH_PREFIX     "/dev/block/by-name/"
#define EXT4_PATH_PREFIX      "/sys/fs/ext4/"
#define F2FS_PATH_PREFIX      "/sys/fs/f2fs/"
#define DISABLE_BARRIER_ENTRY "disable_barrier"
#define FLUSH_MERGE_ENTRY     "current_flush_merge"
#define TRY_PROPERTY

static char *fs_data_dev = NULL;
static int data_fstype = -1;

void set_f2fs_current_flush_merge(char *dev, int value)
{
    struct stat stat_buf;

    ALOGI("set_f2fs_current_flush_merge:%d", value);

    if (f2fs_flushSupport == -1) {
        f2fs_flushSupport = (0 == stat(dev, &stat_buf)) ? 1 : 0;
        f2fs_flushDefault = get_int_value(dev);
    }

    if (f2fs_flushSupport != 1) {
        return;
    }

    ALOGI("set_f2fs_current_flush_merge set default set:%d", f2fs_flushDefault);
    set_value(dev, f2fs_flushDefault);
}

#define PATH_BLOCK              "/sys/block"
#define PATH_DEVICE_TREE        "/proc/device-tree/soc"
#define PATH_LEN                (100)
#define UFS_HOST_MAX            (2)
#define SD_DEV_MAX              (8)
#define SD_NAME_LEN             (4)
#define VALUE_LEN               (16)

static char ufs_path[UFS_HOST_MAX][PATH_LEN];
static char ufs_auto_hibern8[UFS_HOST_MAX][VALUE_LEN];
static char ufs_irq_affinity[UFS_HOST_MAX][VALUE_LEN];
static uint8_t ufs_host_cnt;

static char sd_dev[SD_DEV_MAX][SD_NAME_LEN];
static char io_sched[SD_DEV_MAX][VALUE_LEN];
static uint8_t sd_dev_cnt;

 /*   return value:
  *         0, error or read nothing
  *        !0, read counts
  */
static int read_from_file(const char *path, char *buf, int size)
{
    if (!path) {
        return 0;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        ALOGE("Could not open '%s'\n", path);
        char *err_str = strerror(errno);
        ALOGD("error : %d, %s\n", errno, err_str);
        return 0;
    }

    int count = read(fd, buf, size);
    if (count > 0) {
        count = (count < size) ? count : size - 1;
        while (count > 0 && buf[count-1] == '\n') count--;
        buf[count] = '\0';
    } else {
        buf[0] = '\0';
    }

    close(fd);
    return count;
}

static void setup_ufs_path(void)
{
    DIR *d;
    struct dirent *de;
    char base[10] = {0};

    if (ufs_host_cnt)
        return;

    d = opendir(PATH_DEVICE_TREE);
    if (d == 0)
        return;

    while (ufs_host_cnt < UFS_HOST_MAX && (de = readdir(d)) != 0) {
        if (!strncmp("ufshci@", de->d_name, 7)) {
            char sysfs_path[PATH_LEN] = {0};
            char id_path[PATH_LEN] = {0};
            char id_buf[8] = {0};
            unsigned int id = 0;
            struct stat stat_buf;
            int i, cnt;

            if (strlen(de->d_name) != 15) {
                LOG_E(" %d: %s, unexpected format", __LINE__, de->d_name);
                continue;
            }
            strncpy(base, de->d_name + 7, 8);
            base[8] = '\0';

            if (snprintf(sysfs_path, PATH_LEN,
                         "sys/devices/platform/soc/%s.ufshci", base) < 0) {
                LOG_E(" %d: snprintf failed", __LINE__);
                continue;
            }

            if (stat(sysfs_path, &stat_buf)) {
                LOG_I("%s doesn't exist", de->d_name);
                continue;
            }

            if (snprintf(id_path, PATH_LEN, "%s/of_node/id", sysfs_path) < 0) {
                LOG_E("%d: snprintf failed", __LINE__);
                continue;
            }

            cnt = read_from_file(id_path, id_buf, sizeof(id_buf));
            if (!cnt || cnt != 4) {
                LOG_E("read %s error", id_path);
                for (i = 0; i < cnt; i++) {
                    LOG_E("ufs_id[%d] = 0x%2x", i, id_buf[i]);
                }
                continue;
            }

            id = (((uint8_t)id_buf[3]) + (((uint8_t)id_buf[2]) << 8) +
                 (((uint8_t)id_buf[1]) << 16) + (((uint8_t)id_buf[0]) << 24));
            if (id >= UFS_HOST_MAX) {
                LOG_E("%s = %u, invalid value", id_path, id);
                continue;
            }

            if (ufs_path[id][0]) {
                LOG_E("ufs_path[%u] has already been set", id);
                continue;
            }

            if (snprintf(ufs_path[id], PATH_LEN, "%s", sysfs_path) < 0) {
                LOG_E("%d: snprintf failed", __LINE__);
                continue;
            }
            ufs_host_cnt++;
        }
    }
    closedir(d);
}

static void get_ufs_setting(unsigned int id, const char *name, char *value,
                            size_t len)
{
    struct stat stat_buf;
    char path[PATH_LEN] = {0};

    if (!name || !value) {
        LOG_E("name or value invalid");
        return;
    }

    setup_ufs_path();

    if (id >= ufs_host_cnt) {
        LOG_I("ufs%u doesn't exist", id);
        return;
    }

    if (strlen(ufs_path[id]) + strlen(name) >= PATH_LEN - 1) {
        LOG_E("path length overflow");
        return;
    }

    if (snprintf(path, PATH_LEN, "%s%s", ufs_path[id], name) < 0) {
        LOG_E("%d: snprintf failed", __LINE__);
        return;
    }

    if (stat(path, &stat_buf) != 0)
        return;

    read_from_file(path, value, len);
}

static void get_io_scheduler(void)
{
    struct dirent *de;
    DIR *d;
    struct stat stat_buf;
    char path[PATH_LEN] = {0}, buf[512] = " ", *str;
    int i;

    d = opendir(PATH_BLOCK);
    if (d == 0)
        return;
    while (sd_dev_cnt < SD_DEV_MAX && (de = readdir(d)) != 0) {
        if ((strlen(de->d_name) == 3) && !strncmp("sd", de->d_name, 2)) {
            strncpy(sd_dev[sd_dev_cnt], de->d_name, SD_NAME_LEN - 1);
            if (snprintf(path, PATH_LEN, "%s/%s/queue/scheduler",
                         PATH_BLOCK, de->d_name) < 0) {
                LOG_E("%d: snprintf failed", __LINE__);
                continue;
            }

            if (!read_from_file(path, buf + 1, sizeof(buf) - 1))
                continue;

            if (!strtok(buf, "["))
                continue;

            str = strtok(NULL, "]");
            if (!str)
                continue;

            strncpy(io_sched[sd_dev_cnt], str, VALUE_LEN - 1);
            sd_dev_cnt++;
        }
    }
    closedir(d);
}

int init_ufs_auto_hibern8(int power_on)
{
    get_ufs_setting(0, "/auto_hibern8", ufs_auto_hibern8[0], VALUE_LEN);
    return 0;
}

int init_ufs_irq_affinity(int power_on)
{
    get_ufs_setting(0, "/irq/smp_affinity", ufs_irq_affinity[0], VALUE_LEN);
    return 0;
}

int init_ufs1_auto_hibern8(int power_on)
{
    get_ufs_setting(1, "/auto_hibern8", ufs_auto_hibern8[1], VALUE_LEN);
    return 0;
}

int init_ufs1_irq_affinity(int power_on)
{
    get_ufs_setting(1, "/irq/smp_affinity", ufs_irq_affinity[1], VALUE_LEN);
    return 0;
}

int init_io_scheduler(int power_on)
{
    get_io_scheduler();
    return 0;
}

static int __set_ufs_auto_hibern8(unsigned int id, int value)
{
    char path[PATH_LEN] = {0};
    const char *name = "/auto_hibern8";

    if (id >= ufs_host_cnt) {
        LOG_I("ufs%u doesn't exist", id);
        return 0;
    }

    if (!strlen(ufs_path[id])) {
        LOG_E("ufs_path[%u] doesn't exist", id);
        return -1;
    } else if (strlen(ufs_path[id]) + strlen(name) >= PATH_LEN - 1) {
        LOG_E("path length overflow");
        return -1;
    }

    if (snprintf(path, PATH_LEN, "%s%s", ufs_path[id], name) < 0) {
        LOG_E("%d: snprintf failed", __LINE__);
        return -1;
    }

    if (!value)
        set_value(path, ufs_auto_hibern8[id]);
    else
        set_value(path, value);

    return 0;
}

int set_ufs_auto_hibern8(int value, void *scn)
{
    return __set_ufs_auto_hibern8(0, value);
}

int set_ufs1_auto_hibern8(int value, void *scn)
{
    return __set_ufs_auto_hibern8(1, value);
}

static int __set_ufs_irq_affinity(unsigned int id, int value)
{
    char buf[16];
    char path[PATH_LEN] = {0};
    const char *name = "/irq/smp_affinity";

    if (id >= ufs_host_cnt) {
        LOG_I("ufs%u doesn't exist", id);
        return 0;
    }

    if (!strlen(ufs_path[id])) {
        LOG_E("ufs_path[%u] doesn't exist", id);
        return -1;
    } else if (strlen(ufs_path[id]) + strlen(name) >= PATH_LEN - 1) {
        LOG_E("path length overflow");
        return -1;
    }

    if (snprintf(path, PATH_LEN, "%s%s", ufs_path[id], name) < 0) {
        LOG_E("%d: snprintf failed", __LINE__);
        return -1;
    }

    if (!value) {
        set_value(path, ufs_irq_affinity[id]);
    } else {
        if (snprintf(buf, sizeof(buf), "%x", value) < 0) {
            LOG_E("%d: snprintf failed", __LINE__);
            return -1;
        }
        set_value(path, buf);
    }

    return 0;
}

int set_ufs_irq_affinity(int value, void *scn)
{
    return __set_ufs_irq_affinity(0, value);
}

int set_ufs1_irq_affinity(int value, void *scn)
{
    return __set_ufs_irq_affinity(1, value);
}

int set_io_scheduler(int value, void *scn)
{
    char path[PATH_LEN] = {0};
    int i;

    for (i = 0; i < sd_dev_cnt; i++) {
        if (snprintf(path, PATH_LEN, "%s/%s/queue/scheduler",
                     PATH_BLOCK, sd_dev[i]) < 0) {
            LOG_E("%d: snprintf failed", __LINE__);
            continue;
        }

        switch (value) {
        case 0:
            set_value(path, io_sched[i]);
            break;

        case 1:
            set_value(path, "bfq");
            break;

        case 2:
            set_value(path, "kyber");
            break;

        case 3:
            set_value(path, "mq-deadline");
            break;

        case 4:
            set_value(path, "none");
            break;

        default:
            break;
        }
    }

    return 0;
}

static int __set_ufs_tracing_mode(unsigned int id, int value)
{
    char tp_path[PATH_LEN] = {0};
    char btag_path[PATH_LEN] = {0};
    const char *tp_node = "/dbg_tp_unregister";
    const char *btag_node = "/skip_blocktag";

    if (id >= ufs_host_cnt) {
        LOG_I("%s: ufs%u doesn't exist", __func__, id);
        return 0;
    }

    if (!strlen(ufs_path[id])) {
        LOG_E("ufs_path[%u] doesn't exist", id);
        return -1;
    } else if (strlen(ufs_path[id]) + strlen(tp_node) >= PATH_LEN - 1) {
        LOG_E("tp path length overflow");
        return -1;
    } else if (strlen(ufs_path[id]) + strlen(btag_node) >= PATH_LEN - 1) {
        LOG_E("btag path length overflow");
        return -1;
    }

    if (snprintf(tp_path, PATH_LEN, "%s%s", ufs_path[id], tp_node) < 0) {
        LOG_E("%d: snprintf failed", __LINE__);
        return -1;
    }

    if (snprintf(btag_path, PATH_LEN, "%s%s", ufs_path[id], btag_node) < 0) {
        LOG_E("%d: snprintf failed", __LINE__);
        return -1;
    }

    switch (value) {
    case 0:
        set_value(tp_path, 0);
        set_value(btag_path, 0);
        break;

    case 1:
        set_value(tp_path, 1);
        break;

    case 2:
        set_value(btag_path, 1);
        break;

    default:
        return -1;
    }

    return 0;
}

int set_ufs_tracing_mode(int value, void *scn)
{
    return __set_ufs_tracing_mode(0, value);
}

int set_ufs1_tracing_mode(int value, void *scn)
{
    return __set_ufs_tracing_mode(1, value);
}
