#include <dlfcn.h>
#include <stdio.h>
#include <log/log.h>
#include <utils/Log.h>
#include <cutils/uevent.h>
#include <mtkperf_resource.h>
#include <cutils/properties.h>
#include <utils/Timers.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <string>

#define LOG_TAG "touch_boost"
#ifdef ALOGI
#undef ALOGI
#define ALOGI(...) do {((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__));} while(0)
#endif

#define PATH_TOUCH_BOOST_IOCTL "/proc/perfmgr_touch_boost/ioctl_touch_boost"
#define ENABLE 1
#define BOOST_DURATION 100
#define ACTIVE_TIME 100
#define DEBOOST_WHEN_RENDER 1
#define IDLEPREFER_TA 1
#define IDLEPREFER_FG 1
#define UTIL_TA 1
#define UTIL_FG 1
#define CPUFREQ_C0 -1
#define CPUFREQ_C1 -1
#define CPUFREQ_C2 -1
#define BOOST_UP 0
#define BOOST_DOWN 1
#define TA_GRP_AWARE_BOOST_DURATION 2000
#define TA_GRP_AWARE_BOOST_THRESHOLD 5

enum {
    TOUCH_BOOST_UNKNOWN = -1,
    TOUCH_BOOST_CPU = 0,
};

struct _TOUCH_BOOST_PACKAGE {
    __s32 cmd;
    __s32 enable;
    __s32 boost_duration;
    __s32 active_time;
    __s32 deboost_when_render;
    __s32 idleprefer_ta;
    __s32 idleprefer_fg;
    __s32 util_ta;
    __s32 util_fg;
    __s32 cpufreq_c0;
    __s32 cpufreq_c1;
    __s32 cpufreq_c2;
    __s32 boost_up;
    __s32 boost_down;
};

#define TOUCH_BOOST_GET_CMD _IOW('g', 1, struct _TOUCH_BOOST_PACKAGE)
#define PERF_LOCK_LIB_FULL_NAME  "libmtkperf_client_vendor.so"

typedef int (*perf_lock_acq)(int, int, int[], int);
typedef int (*perf_lock_rel)(int);

static int  (*perfLockAcq)(int, int, int[], int) = NULL;
static int  (*perfLockRel)(int) = NULL;

void *lib_handle = NULL;
static int t_hdl = 0;
static int ta_hdl = 0;
static int devfd_tch = -1;

int check_touch_boost_ioctl_valid() {
    if (devfd_tch >= 0) {
        return 0;
    } else if (devfd_tch == -1) {
        devfd_tch = open(PATH_TOUCH_BOOST_IOCTL, O_RDONLY);
        if (devfd_tch < 0 && errno == ENOENT) {
            devfd_tch = -2;
        }
        if (devfd_tch == -1) {
            ALOGE("Cannot open %s (%s)", PATH_TOUCH_BOOST_IOCTL, strerror(errno));
            return -1;
        }
    } else if (devfd_tch == -2) {
        return -2;
    }
    return 0;
}

struct _TOUCH_BOOST_PACKAGE TouchBoostGetCmd(void) {
    int ioctl_ret = 0;
    int ioctl_valid = check_touch_boost_ioctl_valid();
    _TOUCH_BOOST_PACKAGE msg;
    msg.cmd = -1;
    msg.enable = ENABLE;
    msg.boost_duration = BOOST_DURATION;
    msg.active_time = ACTIVE_TIME;
    msg.deboost_when_render = DEBOOST_WHEN_RENDER;
    msg.idleprefer_ta = IDLEPREFER_TA;
    msg.idleprefer_fg = IDLEPREFER_FG;
    msg.util_ta = UTIL_TA;
    msg.util_fg = UTIL_FG;
    msg.cpufreq_c0 = CPUFREQ_C0;
    msg.cpufreq_c1 = CPUFREQ_C1;
    msg.cpufreq_c2 = CPUFREQ_C2;
    msg.boost_up = BOOST_UP;
    msg.boost_down = BOOST_DOWN;

    if (ioctl_valid == 0) {
        ioctl_ret = ioctl(devfd_tch, TOUCH_BOOST_GET_CMD, &msg);
    } else {
        ALOGD("%s ioctl_valid:%d", __func__, ioctl_valid);
        sleep(60);
    }

    if (ioctl_ret != 0) {
        ALOGD("%s ioctl_ret:%d", __func__, ioctl_ret);
        sleep(60);
    }

    return msg;
}

static int load_perf_api(void) {
    void *func;

    lib_handle = dlopen(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);

    if (lib_handle == NULL) {
        ALOGD("power hal: dlopen fail: %s\n", dlerror());
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_acq");
    perfLockAcq = reinterpret_cast<perf_lock_acq>(func);

    if (perfLockAcq == NULL) {
        ALOGD("power hal: perfLockAcq error: %s\n", dlerror());
        dlclose(lib_handle);
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_rel");
    perfLockRel = reinterpret_cast<perf_lock_rel>(func);

    if (perfLockRel == NULL) {
        ALOGD("power hal: perfLockRel error: %s\n", dlerror());
        dlclose(lib_handle);
        return -1;
    }

    return 0;
}

static int set_top_grp_aware() {

    ALOGI("set_top_grp_aware");

    int perf_lock_rsc[] = {
        PERF_RES_POWERHAL_TOUCH_BOOST_TA_GAS_ENABLE, 1,
        PERF_RES_POWERHAL_TOUCH_BOOST_TA_GAS_THRESHOLD, TA_GRP_AWARE_BOOST_THRESHOLD
        };
    int size = sizeof(perf_lock_rsc)/sizeof(int);

    if (!perfLockAcq || !perfLockRel)
        load_perf_api();

    ta_hdl = perfLockAcq(ta_hdl, TA_GRP_AWARE_BOOST_DURATION, perf_lock_rsc, size);

    return 0;
}


static int touch_boost(int enable,
    int deboost_when_render,
    int active_time,
    int boost_duration,
    int idleprefer_ta, int idleprefer_fg,
    int util_ta, int util_fg,
    int cpufreq_c0, int cpufreq_c1, int cpufreq_c2,
    int boost_up, int boost_down) {

    ALOGI("enable=%d, boost_up=%d, boost_down=%d, render_deboost=%d, active_time=%d, duration=%d, idleprefer(TA,FG)=(%d,%d), util(TA,FG)=(%d,%d), freq(c0,c1,c2)=(%d,%d,%d)",
        enable, boost_up, boost_down, deboost_when_render, active_time, boost_duration,
        idleprefer_ta, idleprefer_fg, util_ta, util_fg,
        cpufreq_c0, cpufreq_c1, cpufreq_c2);

    int perf_lock_rsc[] = {
        PERF_RES_SCHED_PREFER_IDLE_TA, idleprefer_ta,
        PERF_RES_SCHED_PREFER_IDLE_FG, idleprefer_fg,
        PERF_RES_SCHED_UCLAMP_MIN_TA, util_ta,
        PERF_RES_SCHED_UCLAMP_MIN_FG, util_fg,
        PERF_RES_CPUFREQ_MIN_CLUSTER_0, cpufreq_c0,
        PERF_RES_CPUFREQ_MIN_CLUSTER_1, cpufreq_c1,
        PERF_RES_CPUFREQ_MIN_CLUSTER_2, cpufreq_c2,
        };
    int size = sizeof(perf_lock_rsc)/sizeof(int);

    if (!perfLockAcq || !perfLockRel)
        load_perf_api();

    if (enable > 0) {
        t_hdl = perfLockAcq(t_hdl, boost_duration, perf_lock_rsc, size);
    } else {
        perfLockRel(t_hdl);
        t_hdl = 0;
    }

    return 0;
}

int main(void) {

    ALOGI("start touch boost service.");

    struct _TOUCH_BOOST_PACKAGE msg;

    while (1) {
        msg = TouchBoostGetCmd();
        switch (msg.cmd) {
            case TOUCH_BOOST_UNKNOWN:
                ALOGD("TOUCH_BOOST_UNKNOWN cmd:%d", msg.cmd);
                sleep(60);
                break;
            case TOUCH_BOOST_CPU:
                touch_boost(msg.enable,
                    msg.deboost_when_render,
                    msg.active_time,
                    msg.boost_duration,
                    msg.idleprefer_ta, msg.idleprefer_fg,
                    msg.util_ta, msg.util_fg,
                    msg.cpufreq_c0, msg.cpufreq_c1, msg.cpufreq_c2,
                    msg.boost_up, msg.boost_down);

                    set_top_grp_aware();
                break;
            default:
                ALOGD("default cmd:%d", msg.cmd);
                break;
        }
    }

    return 0;
}