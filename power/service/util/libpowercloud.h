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

#ifndef __LIBPOWERHAL_CLOUD_H__
#define __LIBPOWERHAL_CLOUD_H__

#define APP_LIST_FILE "power_app_cfg.xml"
#define CON_TBL_FILE  "powercontable.xml"
#define FSTB_TBL_FILE "fstb.cfg"
#define POWER_MODE_FILE "power_mode.cfg"
#define SCN_TBL_FILE "powerscntbl.xml"
#define XGF_TBL_FILE "xgf.cfg"
//SPD:add appthermalmode clound support by yifan.hou 20240528 start
#define APP_THERMAL_LIST_FILE "power_app_thermal_cfg.xml"
//SPD:add appthermalmode clound support by yifan.hou 20240528 end

#define DATA_VENDOR_APP_PATH "/data/vendor/powerhal/power_app_cfg.xml"
#define DATA_VENDOR_CON_PATH "/data/vendor/powerhal/powercontable.xml"
#define DATA_VENDOR_FSTB_PATH "/data/vendor/powerhal/fstb.cfg"
#define DATA_VENDOR_MODE_PATH  "/data/vendor/powerhal/power_mode.cfg"
#define DATA_VENDOR_SCN_PATH  "/data/vendor/powerhal/powerscntbl.xml"
#define DATA_VENDOR_XGF_PATH  "/data/vendor/powerhal/xgf.cfg"
//SPD:add appthermalmode clound support by yifan.hou 20240604 start
#define DATA_VENDOR_APP_THERMAL_PATH  "/data/vendor/powerhal/power_app_thermal_cfg.xml"
//SPD:add appthermalmode clound support by yifan.hou 20240604 end

#define SIZE_1_K_BYTES     1024
#define TRAN_POWERHAL_CLOUD_PROP "ro.odm.tr_powerhal.cloud.feature.support"
#define TRAN_POWERHAL_ENCODE_PROP "ro.odm.tr_powerhal.encode.feature.support"

extern void registCloudctlListener() ;
extern void unregistCloudctlListener();
#endif
