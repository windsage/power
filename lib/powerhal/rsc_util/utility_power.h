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

 #ifndef ANDROID_UTILITY_POWER_H
 #define ANDROID_UTILITY_POWER_H

#define PATH_POWER_MODE_FILE       "/vendor/etc/power_mode.cfg"
#define PATH_POWER_MODE_FILE_2     "/data/vendor/powerhal/power_mode.cfg"
#define TRAN_MAX_MODE	6


extern int default_core_max[TRAN_MAX_MODE][3];
extern void init_power_mode(void);
 #endif // ANDROID_UTILITY_POWER_H