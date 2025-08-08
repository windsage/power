/**
  *Copyright © 2023 Transsion Inc
  *Author sifeng.tian
  *include compute the frequency limit under high temperature
  */

#ifndef _UTILITY_THERMAL_UX_H
#define _UTILITY_THERMAL_UX_H


enum clusterType{
    CLUSTER_L,
    CLUSTER_B,
    CLUSTER_BB,
    CLUSTER_CUR_MAX,
};

void thermalUxPolicyInit(int clusterNum);
//SPD: add powerhal reinit by sifengtian 20230711 start
void thermalUxPolicyReInit();
//SPD: add powerhal reinit by sifengtian 20230711 end
bool is_valid_temp();

#endif