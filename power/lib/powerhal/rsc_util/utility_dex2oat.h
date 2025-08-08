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

#ifndef ANDROID_UTILITY_DEX2OAT_H
#define ANDROID_UTILITY_DEX2OAT_H

extern int set_thermal_dex2oat_cpus(int value,void *scn);
extern int unset_thermal_dex2oat_cpus(int value,void *scn);
extern int set_cpuset_dex2oat_cpus(int value, void *scn);
extern int unset_cpuset_dex2oat_cpus(int value, void *scn);
extern int init_dex2oat_cpus(int power_on);

#endif
