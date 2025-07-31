#ifndef ANDROID_SBE_HANDLE_H
#define ANDROID_SBE_HANDLE_H

#include <linux/ioctl.h>
#define SMART_LAUNCH_ALGORITHM                  _IOW('g', 1, struct _SMART_LAUNCH_PACKAGE)

#define PRESSURE_PATH_MEMORY	"/proc/pressure/memory"
#define PRESSURE_PATH_IO	    "/proc/pressure/io"
#define PRESSURE_PATH_CPU	    "/proc/pressure/cpu"

#define MEM_THRESHOLD_DEFAULT		7.0
#define CPU_THRESHOLD_DEFAULT		20.0
#define IO_THRESHOLD_DEFAULT		20.0

enum pressure_type {
    PSI_TYPE_SOME,
    PSI_TYPE_FULL,
    PSI_TYPE_COUNT
};

struct pressure_stats {
    double avg10;
    double avg60;
    double avg300;
    unsigned long long total;
};

struct pressure_data {
    const char* filename;
    struct pressure_stats stats[PSI_TYPE_COUNT];
};

struct system_pressure_info {
    float mem_threshold;
    float cpu_threshold;
    float io_threshold;
    struct pressure_data mem_data;
    struct pressure_data cpu_data;
    struct pressure_data io_data;
};

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
int notifySmartLaunchEngine(const char *packinfo);
#endif

