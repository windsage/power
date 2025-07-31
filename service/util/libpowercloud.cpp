/* Copyright Statement:
 *
 *
 * Transsion Inc. (C) 2010. All rights reserved.
 *
 *
 * The following software/firmware and/or related documentation ("Transsion
 * Software") have been modified by Transsion Inc. All revisions are subject to
 * any receiver's applicable license agreements with Transsion Inc.
 */
#define LOG_TAG "libPowerCloud"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/stat.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <tranlog/libtranlog.h>
#include "libpowercloud.h"
//SPD: add powerhal reinit by sifengtian 20230711 start
#include "libpowerhal_wrap.h"
//SPD: add powerhal reinit by sifengtian 20230711 end

char *POWER_CLDCTL_ID = "1001000034";
char *POWER_CLDCTL_VER = "v1.0";
char *POWER_FILE_TYPE = "f_type";
char *POWER_FILE_ENCODE = "f_encode"; //Indicates whether the configuration file is encrypted
static int XGF_MASK = 0b00000001;
static int SCN_MASK = 0b00000010;
static int POWER_MODE_MASK = 0b00000100;
//SPD:add appthermalmode clound support by yifan.hou 20240604 start
static int APP_THERMAL_MASK = 0b10000000;
//SPD:add appthermalmode clound support by yifan.hou 20240604 end
static int FSTB_MASK = 0b00010000;
static int CON_MASK = 0b00100000;
static int APP_MASK = 0b01000000;
static int encode_support = 0;
static int cloud_support = 0;
//SPD:porting thermal ux policy by sifengtian 20230525 start
char *THERMAL_UX_TEMP_MAX_CLOUD_KEY = "thermal_ux_temp_max";
char *THERMAL_UX_TEMP_MIN_CLOUD_KEY = "thermal_ux_temp_min";
char * THERMAL_UX_TEMP_MAX_CLOUD_VALUE = NULL;
char * THERMAL_UX_TEMP_MIN_CLOUD_VALUE = NULL;
#define THERMAL_UX_TEMP_MAX_CLOUD_PROP "persist.vendor.powerhal.thermal_ux_temp_max"
#define THERMAL_UX_TEMP_MIN_CLOUD_PROP "persist.vendor.powerhal.thermal_ux_temp_min"

static void update_thermal_ux_temp_threshold()
{
    if (THERMAL_UX_TEMP_MAX_CLOUD_VALUE) {
        property_set(THERMAL_UX_TEMP_MAX_CLOUD_PROP, THERMAL_UX_TEMP_MAX_CLOUD_VALUE);
    }

    if (THERMAL_UX_TEMP_MIN_CLOUD_VALUE) {
        property_set(THERMAL_UX_TEMP_MIN_CLOUD_PROP, THERMAL_UX_TEMP_MIN_CLOUD_VALUE);
    }
}
//SPD:porting thermal ux policy by sifengtian 20230525 end

static int check_property_value(char *prop)
{
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    int prop_value = 0;

    if(prop == NULL)
        return 0;

    property_get(prop, prop_content, "0");
    prop_value = atoi(prop_content);

    return prop_value;
}

static int power_cloud_copy_file(char* src, const char* dest)
{
	int fd1, fd2;
	int file_size, buff_size;
	int ret = -1;
	char* buff = NULL ;

	fd1 = open(src, O_RDWR);
	if(fd1 < 0) {
		ALOGE("open %s failed !",src);
		ret = -1;
		return ret;
	}
	fd2 = open(dest, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if(fd2 < 0) {
		ALOGE("open %s failed !",dest);
		close(fd1);
		ret = -1;
		return ret;
	}
	ret = chmod(dest, 0664);
	if(ret < 0) {
		ALOGE("chmod %s failed ! ret = %d", dest, ret);
		return ret;
	}

	file_size = lseek(fd1, 0, SEEK_END);
	ALOGD("file_size = %d ",file_size);
	lseek(fd1, 0, SEEK_SET);

	buff = (char *)malloc(SIZE_1_K_BYTES);
	if (buff == NULL) {
		ALOGE("fpsmgr:buff malloc fail!");
		ret = -1;
		goto out;
	}

	while(file_size > 0) {
		memset(buff, 0, sizeof(buff));
		if(file_size > SIZE_1_K_BYTES) {
			buff_size = SIZE_1_K_BYTES;
		} else {
			buff_size = file_size;
		}
		ret = read(fd1, buff, buff_size);
		if (ret < 0) {
			ALOGE("read %s failed ! ret =%d",src, ret);
			break;
		}
		ret = write(fd2, buff, buff_size);
		if (ret < 0) {
			ALOGE("write %s failed ! ret =%d",dest, ret);
			break;
		}
		file_size -= SIZE_1_K_BYTES;
	}

	free(buff);
out:
	close(fd1);
	close(fd2);
	return ret;
}

static char* getSrcFilePath(char* filePath, const char* file_name)
{
	char* fileSrcPath = NULL;

	int length = strlen(filePath) + sizeof("/") + strlen(file_name) + sizeof("");
	fileSrcPath = (char*) malloc(length);
	if(fileSrcPath == NULL) {
		ALOGE("alloc mem failed!!");
		goto ERROR;
	}
	sprintf(fileSrcPath, "%s/%s", filePath, file_name);
	fileSrcPath[length-1] = '\0';
ERROR:
	return fileSrcPath;
}


static void do_file_copy(char *src_path, char *dest_path)
{
    struct stat stat_buf;

    if(!src_path) {
        ALOGE("path is null!");
        return;
    }

    if(!dest_path) {
        ALOGE("dest_path is null!");
        free(src_path);
        return;
    }

    if (0 == stat(src_path, &stat_buf))
    {
        ALOGI("src_path =%s",src_path);
        power_cloud_copy_file(src_path, dest_path);
        free(src_path);
    } else {
        free(src_path);
    }
}

/*
The file mask corresponds to the file to be updated
app_thermal app con fstb app_fling power_mode scn xgf 
 0           0    0   0    0            0       0   1   //=1
 0           0    0   0    0            0       1   0   //=2
 0           0    0   0    0            1       0   0   //=4
 0           0    0   0    1            0       0   0   //=8
 0           0    0   1    0            0       0   0   //=16
 0           0    1   0    0            0       0   0   //=32
 0           1    0   0    0            0       0   0   //=64
 1           0    0   0    0            0       0   0   //=128
 .........................................................
 1           1    1   1    1            1       1   1   //=256

ex:
app con fstb app_fling power_mode scn xgf
 1    1   0    0            1      1   0   //=102
Cloud command:
f_encode:Y ----->file encrypted
{"v":"v1.1","m":"n","t":"20220916","e":true,"f":"http://cdn.shalltry.com/public/OSFeature_test/file/1663306698367.zip","f_type":"102","f_encode":"Y"}
f_encode:N
{"v":"v1.1","m":"n","t":"20220916","e":true,"f":"http://cdn.shalltry.com/public/OSFeature_test/file/1663306698367.zip","f_type":"102","f_encode":"N"}
*/

static void power_cloudctl_data_callback(char* key) {
	char* content = getConfig(POWER_CLDCTL_ID);
	char* file_encode =NULL;
	char* file_type =NULL;
	char* filePath = NULL;
	ALOGI("content = %s", content);
        //SPD:fix getString error Controls by sifengtian 20230612 start
        //SPD:porting thermal ux policy by sifengtian 20230525 start
        char* thermal_ux_temp_min = NULL;
        //SPD:porting thermal ux policy by sifengtian 20230525 end
        char* thermal_ux_temp_max = NULL;
        //SPD:fix getString error Controls by sifengtian 20230612 end

	if(content) {
        /*update file*/
        file_encode = getString(content, POWER_FILE_ENCODE);
        ALOGI("file_encode = %s,encode_support = %d", file_encode,encode_support);
        if(!file_encode) {
            ALOGE("file_encode is fail!");
            goto OUT;
        }

        if(!strcmp(file_encode, "Y") && !encode_support) {
            ALOGD("project need not encode_support!");
            goto OUT;
        }else if(!strcmp(file_encode, "N") && encode_support) {
            ALOGD("project need encode_support!");
            goto OUT;
        }

        file_type = getString(content, POWER_FILE_TYPE);
        if(!file_type) {
            ALOGE("file_type is fail!");
            goto OUT;
        }

        int file_num = atoi(file_type);
        ALOGI("file_num = %d", file_num);

        filePath = getFilePath(POWER_CLDCTL_ID);
        if(!filePath) {
            ALOGE("filePath is fail!");
            goto OUT;
        }

        ALOGI("filepath =%s", filePath);

        //SPD:add appthermalmode clound support by yifan.hou 20240604 start
        if(file_num & APP_THERMAL_MASK) {
            char* src_app_thermal_file_path = getSrcFilePath(filePath, APP_THERMAL_LIST_FILE);
            do_file_copy(src_app_thermal_file_path, DATA_VENDOR_APP_THERMAL_PATH);
        }
        //SPD:add appthermalmode clound support by yifan.hou 20240604 end

        if(file_num & XGF_MASK) {
            char* src_xgf_file_path = getSrcFilePath(filePath, XGF_TBL_FILE);
            do_file_copy(src_xgf_file_path, DATA_VENDOR_XGF_PATH);
        }

        if(file_num & SCN_MASK) {
            char* src_scn_file_path = getSrcFilePath(filePath, SCN_TBL_FILE);
            do_file_copy(src_scn_file_path, DATA_VENDOR_SCN_PATH);
        }

        if(file_num & POWER_MODE_MASK) {
            char* src_mode_file_path = getSrcFilePath(filePath, POWER_MODE_FILE);
            do_file_copy(src_mode_file_path, DATA_VENDOR_MODE_PATH);
        }

        if(file_num & FSTB_MASK) {
            char* src_fstb_file_path = getSrcFilePath(filePath, FSTB_TBL_FILE);
            do_file_copy(src_fstb_file_path, DATA_VENDOR_FSTB_PATH);
        }

        if(file_num & CON_MASK) {
            char* src_con_file_path = getSrcFilePath(filePath, CON_TBL_FILE);
            do_file_copy(src_con_file_path, DATA_VENDOR_CON_PATH);
        }

        if(file_num & APP_MASK) {
            char* src_app_file_path = getSrcFilePath(filePath, APP_LIST_FILE);
            do_file_copy(src_app_file_path, DATA_VENDOR_APP_PATH);
        }

        //SPD:fix getString error Controls by sifengtian 20230612 start
        //SPD:porting thermal ux policy by sifengtian 20230525 start
        /* update thermal_ux policy temp Threshold*/
        thermal_ux_temp_max = getString(content, THERMAL_UX_TEMP_MAX_CLOUD_KEY);
        if (!thermal_ux_temp_max) {
            ALOGE("get thermal ux temp is fail!");
            goto OUT;
        }
        THERMAL_UX_TEMP_MAX_CLOUD_VALUE = thermal_ux_temp_max;
        ALOGI("thermal_ux_temp max = %s", THERMAL_UX_TEMP_MAX_CLOUD_VALUE);

        thermal_ux_temp_min = getString(content, THERMAL_UX_TEMP_MIN_CLOUD_KEY);
        if (!thermal_ux_temp_min) {
            ALOGE("get thermal ux temp is fail!");
            goto OUT;
        }
        THERMAL_UX_TEMP_MIN_CLOUD_VALUE = thermal_ux_temp_min;
        ALOGI("thermal_ux_temp min = %s", THERMAL_UX_TEMP_MIN_CLOUD_VALUE);

        update_thermal_ux_temp_threshold();
        //SPD:porting thermal ux policy by sifengtian 20230525 end
        //SPD:fix getString error Controls by sifengtian 20230612 end
	}
    //SPD: add powerhal reinit by sifengtian 20230711 start
    libpowerhal_wrap_ReInit(1);
    //SPD: add powerhal reinit by sifengtian 20230711 end
OUT:
    if(file_type) {
        free(file_type);
    }

    if(filePath) {
        free(filePath);
    }

    if(file_encode) {
        free(file_encode);
    }

    if(content) {
		free(content);
    }
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    if (thermal_ux_temp_min) {
        free(thermal_ux_temp_min);
    }
    //SPD:porting thermal ux policy by sifengtian 20230525 end

    //SPD:fix getString error Controls by sifengtian 20230612 start
    if (thermal_ux_temp_max) {
        free(thermal_ux_temp_max);
    }
    //SPD:fix getString error Controls by sifengtian 20230612 end

	feedBack(POWER_CLDCTL_ID, 1);
}

static struct config_notify power_cloudctl_notify = {
	.notify = power_cloudctl_data_callback
};

void registCloudctlListener() {
    cloud_support = check_property_value(TRAN_POWERHAL_CLOUD_PROP);
    encode_support = check_property_value(TRAN_POWERHAL_ENCODE_PROP);

    if(!cloud_support) {
        ALOGD("powerhal doesn't support cloudEnigne");
        return;
    }

    int ret = startListener(&power_cloudctl_notify, POWER_CLDCTL_ID, POWER_CLDCTL_VER);
    if(ret == 0) {
        ALOGE("registCloudctlListener failed!");
    }
}

void unregistCloudctlListener() {
    if(!cloud_support) {
        ALOGD("powerhal doesn't support cloudEnigne");
        return;
    }

    stopListener(POWER_CLDCTL_ID);
}
