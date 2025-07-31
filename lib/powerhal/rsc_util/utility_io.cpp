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

char *get_mount_dev(const char *name, int *fstype)
{
    char device[128] = {0};
    char *buf = NULL, *dev_ptr, *dev_return = NULL;
    const char *bufp;
    int fd = -1, len, len2;
    ssize_t nbytes = 0;

#ifdef TRY_PROPERTY
    if ((property_get("dev.mnt.part.data", device, NULL) >= 0) && (strlen(device) > 0)) {
        dev_return = (char *) malloc(strlen(device)+1);
        if (!dev_return) {
            ALOGE("Fail to allocate %d bytes memory", (int)strlen(device)+1);
            goto out;
        }
        strncpy(dev_return, device, strlen(device)+1);
        //ALOGI("Found dev.mnt.part.data %s, len %d", dev_return, strlen(dev_return));
    }
#endif

    /* Open and read the file contents. */
    fd = open(PROC_MOUNTS_FILENAME, O_RDONLY);
    if (fd < 0)
        goto out;

    /* Since /proc/mounts cannot be seeked to determine length,
     * reading more bytes to test if eof had been reached and re-allocate buffer if necessary
     */
    len = 2048;
    buf = (char *)malloc(len + 1);
    if (!buf) {
        ALOGE("Fail to allocate %d bytes memory for reading " PROC_MOUNTS_FILENAME, len);
        goto out;
    }

    do {
        len2 = read(fd, buf + nbytes, len - nbytes);
        if (len2 < 0) {
            goto out;
        } else if (len2 == len - nbytes) {
            nbytes += len2;
            len *= 2;
            buf = (char *) realloc(buf, len+1);
            if (!buf) {
                ALOGE("Fail to allocate %d bytes memory for reading " PROC_MOUNTS_FILENAME, len);
                goto out;
            }
        } else if (len2 < len - nbytes) {
            nbytes += len2;
            break;
        } else if (len2 > len - nbytes) {
           goto out;
        }
    } while (1);
    buf[nbytes] = '\0';

    /* Parse the contents of the file, which looks like:
     *
     *     # cat /proc/mounts
     *     rootfs / rootfs rw 0 0
     *     /dev/pts /dev/pts devpts rw 0 0
     *     /proc /proc proc rw 0 0
     *     /sys /sys sysfs rw 0 0
     *     /dev/block/mtdblock4 /system yaffs2 rw,nodev,noatime,nodiratime 0 0
     *     /dev/block/mtdblock5 /data yaffs2 rw,nodev,noatime,nodiratime 0 0
     *     /dev/block/mmcblk0p1 /sdcard vfat rw,sync,dirsync,fmask=0000,dmask=0000,codepage=cp437,iocharset=iso8859-1,utf8 0 0
     *
     * The zeroes at the end are dummy placeholder fields to make the
     * output match Linux's /etc/mtab, but don't represent anything here.
     */
    *fstype = -1;
    bufp = buf;
    while (nbytes > 0) {
        char mount_point[64] = {0};
        char filesystem[64] = {0};
        int matches;

        matches = sscanf(bufp, "%63s %63s %63s", device, mount_point, filesystem);

        //ALOGI("%s %s %s, matches %d", device, mount_point, filesystem, matches);
        if (matches == 3) {
            if (!strcmp(mount_point, name)) {
                if (!strcmp(filesystem, "ext4"))
                    *fstype = 0;
                else if (!strcmp(filesystem, "f2fs"))
                    *fstype = 1;
                else {
                    ALOGE("%s partition is neither ext4 nor f2fs", name);
                    goto out;
                }
                //ALOGI("Found %s at %s", name, device);
                dev_ptr = strrchr(device, '/');
                if (dev_ptr)
                    dev_ptr++;
                else
                    dev_ptr = device;
                break;
            }
        }

        /* Eat the line. */
        while (nbytes > 0 && *bufp != '\n') {
            bufp++;
            nbytes--;
        }
        if (nbytes > 0) {
            bufp++;
            nbytes--;
        }
    }

    if (*fstype < 0)
        goto out;

#ifdef TRY_PROPERTY
    if (dev_return)
        goto out;
#endif

    if (strncmp(dev_ptr, "dm-", 3)) {
        len = strlen(BLOCK_PATH_PREFIX)+ strlen(dev_ptr) + 2;
        dev_return = (char *) malloc(len);
        if (!dev_return) {
            ALOGE("Fail to allocate %d bytes memory", len);
            goto out;
        }
        if(sprintf(dev_return, BLOCK_PATH_PREFIX "%s", dev_ptr) < 0) {
            ALOGE("sprintf error");
            goto out;
        }
        memset(device, 0, sizeof(device));
        len2 = readlink(dev_return, device, sizeof(device)-1);
        if (len2 <= 0)
            ALOGI("Fail to readlink from %s", dev_return);
        free(dev_return);
        dev_return = NULL;
        if (len2 <= 0)
            goto out;
        device[len2] = 0;
    }
    //ALOGI("Found %s", device);
    dev_ptr = strrchr(device, '/');
    if (!dev_ptr)
          goto out;
    dev_ptr++;
    dev_return = (char *) malloc(strlen(dev_ptr)+1);
    if (!dev_return) {
        ALOGE("Fail to allocate %d bytes memory", (int)strlen(dev_ptr)+1);
        goto out;
    }
    strcpy(dev_return, dev_ptr);

out:
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    return dev_return;
}

int set_data_fs_boost(int value, void *scn)
{
    char *data_dev = NULL;

    ALOGI("set_data_partition_boost: value:%d, scn:%p", value, scn);
    if (!fs_data_dev) {
        data_dev = get_mount_dev("/data", &data_fstype);
        if (!data_dev) {
            ALOGE("Fail to find mount device of /data");
            return 0;
        }

        if (data_fstype == 0) {
            /* for ext4 */
            fs_data_dev = (char *) malloc(strlen(EXT4_PATH_PREFIX) + strlen(data_dev)+ strlen(DISABLE_BARRIER_ENTRY) + 2);
            if (!fs_data_dev) {
                ALOGE("Fail to allocate memory");
                goto out;
            }
            if(sprintf(fs_data_dev, EXT4_PATH_PREFIX "%s/" DISABLE_BARRIER_ENTRY, data_dev) < 0) {
                ALOGE("sprintf error");
                free(fs_data_dev);
                fs_data_dev = NULL;
                goto out;
            }
        } else if (data_fstype == 1) {
            /* for f2fs */
            fs_data_dev = (char *) malloc(strlen(F2FS_PATH_PREFIX) + strlen(data_dev)+ strlen(FLUSH_MERGE_ENTRY) + 2);
            if (!fs_data_dev) {
                ALOGE("Fail to allocate memory");
                goto out;
            }
            if(sprintf(fs_data_dev, F2FS_PATH_PREFIX "%s/" FLUSH_MERGE_ENTRY, data_dev) < 0) {
                ALOGE("sprintf error");
                free(fs_data_dev);
                fs_data_dev = NULL;
                goto out;
            }
        }
    }
    //ALOGI("data_fs_boot_entry %s\n", fs_data_dev);
    if (data_fstype == 0) {
        if (value != -1) {
            set_value(fs_data_dev, value);
        } else {
            set_value(fs_data_dev, 0);
        }
    } else if (data_fstype == 1) {
        set_f2fs_current_flush_merge(fs_data_dev, value);
    }
out:
    if(data_dev)
        free(data_dev);
    return 0;
}

#define PATH_UFS_AUTO_HIBERN8   "/sys/devices/platform/soc/112b0000.ufshci/auto_hibern8"
#define PATH_UFS_IRQ_AFFINITY   "/sys/devices/platform/soc/112b0000.ufshci/irq/smp_affinity"
#define PATH_UFS_SKIP_BTAG      "/sys/devices/platform/soc/112b0000.ufshci/skip_blocktag"
#define PATH_UFS_DBG_TP_OFF     "/sys/devices/platform/soc/112b0000.ufshci/dbg_tp_unregister"
#define PATH_IO_SCHEDULER       "/sys/block/sdc/queue/scheduler"

static char ufs_auto_hibern8[16] = {0};
static char ufs_irq_affinity[16] = {0};
static char io_scheduler[16] = {0};

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

static void get_ufs_auto_hibern8(void)
{
    struct stat stat_buf;

    if (stat(PATH_UFS_AUTO_HIBERN8, &stat_buf) != 0)
        return;

    read_from_file(PATH_UFS_AUTO_HIBERN8, ufs_auto_hibern8,
		    sizeof(ufs_auto_hibern8));
}

static void get_ufs_irq_affinity(void)
{
    struct stat stat_buf;

    if (stat(PATH_UFS_IRQ_AFFINITY, &stat_buf) != 0)
        return;

    read_from_file(PATH_UFS_IRQ_AFFINITY, ufs_irq_affinity,
		    sizeof(ufs_irq_affinity));
}

static void get_io_scheduler(void)
{
    struct stat stat_buf;
    char buf[512] = " ", *str;

    if (stat(PATH_IO_SCHEDULER, &stat_buf) != 0)
        return;

    if (!read_from_file(PATH_IO_SCHEDULER, buf + 1, sizeof(buf) - 1))
        return;

    if (!strtok(buf, "["))
        return;

    str = strtok(NULL, "]");
    if (!str)
        return;

    strncpy(io_scheduler, str, 16);
    io_scheduler[15] = '\0';
}

int init_ufs_auto_hibern8(int power_on)
{
    get_ufs_auto_hibern8();
    return 0;
}

int init_ufs_irq_affinity(int power_on)
{
    get_ufs_irq_affinity();
    return 0;
}

int init_io_scheduler(int power_on)
{
    get_io_scheduler();
    return 0;
}

int set_ufs_auto_hibern8(int value, void *scn)
{
    if (!value)
        set_value(PATH_UFS_AUTO_HIBERN8, ufs_auto_hibern8);
    else
        set_value(PATH_UFS_AUTO_HIBERN8, value);

    return 0;
}

int set_ufs_irq_affinity(int value, void *scn)
{
    char buf[16];

    if (!value) {
        set_value(PATH_UFS_IRQ_AFFINITY, ufs_irq_affinity);
    } else {
        if (snprintf(buf, sizeof(buf), "%x", value) < 0) {
            LOG_E("snprintf error");
            return -1;
        }
        set_value(PATH_UFS_IRQ_AFFINITY, buf);
    }

    return 0;
}

int set_io_scheduler(int value, void *scn)
{
    switch (value) {
    case 0:
        set_value(PATH_IO_SCHEDULER, io_scheduler);
        break;

    case 1:
        set_value(PATH_IO_SCHEDULER, "bfq");
        break;

    case 2:
        set_value(PATH_IO_SCHEDULER, "kyber");
        break;

    case 3:
        set_value(PATH_IO_SCHEDULER, "mq-deadline");
        break;

    case 4:
        set_value(PATH_IO_SCHEDULER, "none");
        break;

    default:
        break;
    }

    return 0;
}

int set_ufs_tracing_mode(int value, void *scn)
{
    switch (value) {
    case 0:
        set_value(PATH_UFS_DBG_TP_OFF, 0);
        set_value(PATH_UFS_SKIP_BTAG, 0);
        break;

    case 1:
        set_value(PATH_UFS_DBG_TP_OFF, 1);
        break;

    case 2:
        set_value(PATH_UFS_SKIP_BTAG, 1);
        break;

    default:
        return -1;
    }

    return 0;
}
