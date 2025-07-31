#ifndef ANDROID_SBE_HANDLE_H
#define ANDROID_SBE_HANDLE_H

void uxScrollSavePowerWithNoAppList(char* pack_name, char* act_name);
int uxScrollSavePowerWithAppList(char* pack_name, char* act_name, char* sbe_featurename, int idex);

int uxScrollSavePower(int status, void *scn);
int end_default_hold_time(int hindId, void* scn);
int set_ux_sbe_check_point(int value, void* scn);
int init_ux_sbe_check_point(int value);
int set_ux_sbe_horizontal_duration(int value, void* scn);
int init_ux_sbe_horizontal_duration(int value);
int set_ux_sbe_vertical_duration(int value, void* scn);
int init_ux_sbe_vertical_duration(int value);
int set_ux_sbe_sbb_enable_later(int value, void* scn);
int init_ux_sbe_sbb_enable_later(int value);
int set_ux_sbe_enable_freq_floor(int value, void* scn);
int init_ux_sbe_enable_freq_floor(int value);
int set_ux_sbe_frame_decision(int value, void* scn);
int init_ux_sbe_frame_decision(int value);
#endif

