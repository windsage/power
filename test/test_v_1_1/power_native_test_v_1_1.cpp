#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define LOG_TAG "PowerTest"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <sys/un.h>

#include <aidl/vendor/mediatek/hardware/mtkpower/IMtkPowerService.h>
#include <aidl/android/hardware/power/Boost.h>
#include <aidl/android/hardware/power/IPower.h>
#include <aidl/android/hardware/power/Mode.h>
#include <android/binder_manager.h>

#include "mtkperf_resource.h"
#include "mtkpower_types.h"
#include "mtkpower_hint.h"
//#include "power_util.h"
//#include <utils/Timers.h>
#include <string.h>

using ::ndk::SpAIBinder;
using ::aidl::android::hardware::power::Mode;
using ::aidl::android::hardware::power::Boost;

std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService> gIMtkPowerService;
static std::shared_ptr<aidl::android::hardware::power::IPower> gPowerHal_Aidl_;
static const std::string gPowerHal_Aidl_instance =
        std::string(aidl::android::hardware::power::IPower::descriptor) + "/default";
static const std::string gIMtkPowerService_instance =
        std::string(aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::descriptor) + "/default";

#define CAPABILITY_FILE_PATH "/vendor/etc/capability_test"
#define MAX_LINES 100    // Assuming a maximum of 100 lines
#define MAX_COLUMNS 2    // Each line has two values

int perf_lock_cap_rsc[200];
int cap_hdl = 0;

//namespace android {

enum {
    CMD_AOSP_POWER_HINT = 1,
    CMD_AOSP_POWER_HINT_1_1,
    CMD_MTK_POWER_HINT,
    CMD_CUS_POWER_HINT,
    CMD_QUERY_INFO,
    CMD_SET_INFO,
    CMD_SET_INFO_ASYNC,
    CMD_PERF_LOCK_ACQ,
    CMD_PERF_CUS_LOCK_ACQ,
    CMD_PERF_LOCK_REL,
    CMD_UNIT_TEST,
    CMD_PERF_MSG_TEST,
    CMD_AIDL_SET_MODE,
    CMD_AIDL_IS_MODE_SUPPORT,
    CMD_AIDL_SET_BOOST,
    CMD_AIDL_IS_BOOST_SUPPORT,
    CMD_MODE_TEST,
    CMD_GAME_MODE_TEST,
    CMD_SPORT_MODE_TEST,
};


#define PERF_LOCK_LIB_FULL_NAME  "libmtkperf_client_vendor.so"

typedef int (*perf_lock_acq)(int, int, int[], int);
typedef int (*perf_lock_rel)(int);
typedef int (*perf_lock_rel_async)(int);
typedef int (*perf_cus_lock_hint)(int, int);

/* function pointer to perfserv client */
static int  (*perfLockAcq)(int, int, int[], int) = NULL;
static int  (*perfLockRel)(int) = NULL;
static int  (*perfLockRelAsync)(int) = NULL;
static int  (*perfCusLockHint)(int, int) = NULL;

void *lib_handle = NULL;


static int load_perf_api(void)
{
    void *func = NULL;

    lib_handle = dlopen(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);

    if (lib_handle == NULL) {
        char *dlErrStr = dlerror();
        if (dlErrStr != NULL) {
            (void) fprintf(stderr, "dlopen fail: %s\n", dlErrStr);
        }
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_acq");
    perfLockAcq = reinterpret_cast<perf_lock_acq>(func);

    if (perfLockAcq == NULL) {
        char *dlErrStr = dlerror();
        if (dlErrStr != NULL) {
            (void) fprintf(stderr, "dlopen fail: %s\n", dlErrStr);
        }
        dlclose(lib_handle);
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_rel");
    perfLockRel = reinterpret_cast<perf_lock_rel>(func);

    if (perfLockRel == NULL) {
        char *dlErrStr = dlerror();
        if (dlErrStr != NULL) {
            (void) fprintf(stderr, "dlopen fail: %s\n", dlErrStr);
        }
        dlclose(lib_handle);
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_rel_async");
    perfLockRelAsync = reinterpret_cast<perf_lock_rel_async>(func);

    if (perfLockRelAsync == NULL) {
        char *dlErrStr = dlerror();
        if (dlErrStr != NULL) {
            (void) fprintf(stderr, "dlopen fail: %s\n", dlErrStr);
        }
        dlclose(lib_handle);
        return -1;
    }

    func = dlsym(lib_handle, "perf_cus_lock_hint");
    perfCusLockHint = reinterpret_cast<perf_cus_lock_hint>(func);

    if (perfCusLockHint == NULL) {
        char *dlErrStr = dlerror();
        if (dlErrStr != NULL) {
            (void) fprintf(stderr, "dlopen fail: %s\n", dlErrStr);
        }
        dlclose(lib_handle);
        return -1;
    }

    return 0;
}

#if 0
static int test_perfLockAcquire(int hdl, int duration, int list[], int numArgs)
{
    struct tPowerData vPowerData;
    struct tPowerData vRspData;
    struct tPerfLockData vPerfLockData;
    int size;

    size = numArgs;

    vPerfLockData.hdl = hdl;
    vPerfLockData.duration = duration;
    vPerfLockData.size = size;
    memcpy(vPerfLockData.rscList, list, sizeof(int)*size);
    vPerfLockData.reserved = 0;
    vPerfLockData.pid = (int)getpid();
    vPerfLockData.uid = 0;
    vPowerData.msg  = POWER_MSG_PERF_LOCK_ACQ;
    vPowerData.pBuf = (void*)&vPerfLockData;

    vRspData.pBuf = (void*)&vPerfLockData;
    power_msg_perf_lock_acq(&vPowerData, (void **) &vRspData);

    return vPerfLockData.hdl;
}

static int test_perfCusLockHint(int hint, int duration)  {
    struct tPowerData vPowerData;
    struct tPowerData vRspData;
    struct tCusLockData vCusLockData;

    vCusLockData.hdl = 0;
    vCusLockData.duration = duration;
    vCusLockData.hint = hint;
    vCusLockData.pid = (int)getpid();

    vPowerData.msg  = POWER_MSG_CUS_LOCK_HINT;
    vPowerData.pBuf = (void*)&vCusLockData;

    vRspData.pBuf = (void*)&vCusLockData;
    power_msg_cus_lock_hint(&vPowerData, (void **) &vRspData);

    return vCusLockData.hdl;
}

static void test_perfLockRelease(int hdl) {

    struct tPowerData vPowerData;
    struct tPowerData vRspData;
    struct tLockHdlData vLockHdlData;

    vLockHdlData.hdl = hdl;
    vLockHdlData.reserved = 0;
    vPowerData.msg  = POWER_MSG_PERF_LOCK_REL;
    vPowerData.pBuf = (void*)&vLockHdlData;
    vRspData.pBuf = (void*)&vLockHdlData;

    power_msg_perf_lock_rel(&vPowerData, (void *) &vRspData);
}
#endif

static void unit_test(int cmd)
{
    int perf_lock_rsc1[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 3000000, PERF_RES_DRAM_OPP_MIN, 0};
    int perf_lock_rsc2[] = {PERF_RES_CPUFREQ_MAX_CLUSTER_0, 1500000};

    int perf_lock_rsc3[] = {PERF_RES_FPS_FPSGO_RENDER_PID, 123, PERF_RES_FPS_FPSGO_LLF_POLICY_BY_PID, 1};
    int perf_lock_rsc4[] = {PERF_RES_FPS_FPSGO_RENDER_PID, 456, PERF_RES_FPS_FPSGO_LLF_TH_BY_PID, 40};
    int perf_lock_rsc5[] = {PERF_RES_FPS_FPSGO_RENDER_PID, 789, PERF_RES_FPS_FPSGO_LLF_TH_BY_PID, 70};

    int perf_lock_rsc6[] = {PERF_RES_FPS_FPSGO_RENDER_PID, 123, PREF_RES_FPS_FSTB_NOTIFY_FPS_BY_PID, 30};
    int perf_lock_rsc7[] = {PERF_RES_FPS_FPSGO_RENDER_PID, 456, PREF_RES_FPS_FSTB_NOTIFY_FPS_BY_PID, 60};
    int perf_lock_rsc8[] = {PERF_RES_FPS_FPSGO_RENDER_PID, 789, PREF_RES_FPS_FSTB_NOTIFY_FPS_BY_PID, 120};

    int perf_lock_rsc9[] = {PERF_RES_C2PS_UCLAMP_POLICY_MODE, 0};
    int perf_lock_rsc10[] = {PERF_RES_C2PS_UCLAMP_SIMPLE_POLICY_UP_MARGIN, 20};
    int perf_lock_rsc11[] = {PERF_RES_C2PS_UCLAMP_SIMPLE_POLICY_DOWN_MARGIN, 20};
    int perf_lock_rsc12[] = {PERF_RES_C2PS_UCLAMP_SIMPLE_POLICY_BASE_UPDATE_UCLAMP, 20};
    int perf_lock_rsc13[] = {PERF_RES_C2PS_PROC_TIME_WINDOW_SIZE, 30};

    int hdl1 = 0, hdl2 = 0, hdl3 = 0;
    int i;
    int fpsgo_hdl1 = 0, fpsgo_hdl2 = 0, fpsgo_hdl3 = 0;
    int fpsgo_hdl4 = 0, fpsgo_hdl5 = 0, fpsgo_hdl6 = 0;
    int c2ps_hdl1 = 0, c2ps_hdl2 = 0, c2ps_hdl3 = 0;
    int c2ps_hdl4 = 0, c2ps_hdl5 = 0;

    void *mSoHandle = NULL;
    void *func = NULL;
    int (*PowerHal_Wrap_EnableGameMode)(int32_t, int32_t) = NULL;
    typedef int (*enableGameMode)(int32_t, int32_t);

    int (*PowerHal_Wrap_EnableSFLowPowerMode)(int32_t) = NULL;
    typedef int (*enableSFLowPowerMode)(int32_t);

    if(load_perf_api()!=0) {
        fprintf(stderr, "dlopen fail\n");
        return;
    }





    switch(cmd) {
    case 1:

        for(i=0; i<50; i++) {
            hdl1 = perfLockAcq(0, 4000, perf_lock_rsc1, 2);
            printf("perfLockAcq hdl:%d\n", hdl1);
            sleep(1);
            perfLockRel(hdl1);
        }
        break;

    case 2:

        for(i=0; i<300; i++) {
            hdl1 = perfLockAcq(0, 600000, perf_lock_rsc1, 2);
            printf("perfLockAcq hdl:%d\n", hdl1);
            usleep(300000);
            //perfLockRel(hdl1);
        }
        break;

    case 3:

        hdl1 = perfCusLockHint(MTKPOWER_HINT_AUDIO_POWER_DL, 10000);
        printf("perfCusLockHint hdl:%d\n", hdl1);
        sleep(3);
        hdl2 = perfCusLockHint(MTKPOWER_HINT_AUDIO_POWER_DL, 10000);
        printf("perfCusLockHint hdl:%d\n", hdl2);
        sleep(3);
        hdl3 = perfCusLockHint(MTKPOWER_HINT_AUDIO_DISABLE_WIFI_POWER_SAVE, 10000);
        printf("perfCusLockHint hdl:%d\n", hdl3);
        sleep(30);
        break;

    case 4:
        for(i=0; i<50; i++) {
            hdl1 = perfLockAcq(0, 4000, perf_lock_rsc1, 2);
            printf("perfLockAcq hdl:%d\n", hdl1);
            sleep(1);
            perfLockRelAsync(hdl1);
        }
        break;

    case 5:
        printf("Test repeated perfLock\n");
        hdl1 = perfLockAcq(0, 3000, perf_lock_rsc1, 2);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(1);
        hdl1 = perfLockAcq(hdl1, 3000, perf_lock_rsc2, 2);
        printf("perfLockAcq hdl:%d\n", hdl1);
        break;

    case 6:
        printf("Test fpsgo render by pid\n");
        fpsgo_hdl1 = perfLockAcq(0, 1000, perf_lock_rsc3, 4);
        printf("perfLockAcq hdl:%d\n", fpsgo_hdl1);
        fpsgo_hdl2 = perfLockAcq(0, 10000, perf_lock_rsc4, 4);
        printf("perfLockAcq hdl:%d\n", fpsgo_hdl2);
        fpsgo_hdl3 = perfLockAcq(0, 1000, perf_lock_rsc5, 4);
        printf("perfLockAcq hdl:%d\n", fpsgo_hdl3);
        break;

    case 7:
        printf("Test PowerHal_Wrap_EnableGameMode \n");

        mSoHandle = dlopen("libpowerhalwrap_vendor.so", RTLD_NOW);
        if (mSoHandle != NULL) {
            func = dlsym(mSoHandle, "PowerHal_Wrap_EnableGameMode");
            PowerHal_Wrap_EnableGameMode = reinterpret_cast<enableGameMode>(func);
            if (PowerHal_Wrap_EnableGameMode == NULL) {
                printf("load PowerHal_Wrap_EnableGameMode fail!");
            }
        } else {
            printf("libpowerhalwrap dlerror:%s", dlerror());
        }

        if (PowerHal_Wrap_EnableGameMode) {
            PowerHal_Wrap_EnableGameMode(1, 123);
            sleep(5);
            PowerHal_Wrap_EnableGameMode(0, 123);
        }

        break;

    case 8:
        printf("Test fstb notify fps by pid\n");
        fpsgo_hdl4 = perfLockAcq(0, 1000, perf_lock_rsc6, 4);
        printf("perfLockAcq hdl:%d\n", fpsgo_hdl4);
        fpsgo_hdl5 = perfLockAcq(0, 10000, perf_lock_rsc7, 4);
        printf("perfLockAcq hdl:%d\n", fpsgo_hdl5);
        fpsgo_hdl6 = perfLockAcq(0, 1000, perf_lock_rsc8, 4);
        printf("perfLockAcq hdl:%d\n", fpsgo_hdl6);
        break;

    case 9:
        printf("Test SF Low Power Mode \n");

        mSoHandle = dlopen("libpowerhalwrap_vendor.so", RTLD_NOW);
        if (mSoHandle != NULL) {
            func = dlsym(mSoHandle, "PowerHal_Wrap_EnableSFLowPowerMode");
            PowerHal_Wrap_EnableSFLowPowerMode = reinterpret_cast<enableSFLowPowerMode>(func);
            if (PowerHal_Wrap_EnableSFLowPowerMode == NULL) {
                printf("load PowerHal_Wrap_EnableSFLowPowerMode fail!");
            }
        } else {
            printf("libpowerhalwrap dlerror: %s", dlerror());
        }

        if (PowerHal_Wrap_EnableSFLowPowerMode) {
            PowerHal_Wrap_EnableSFLowPowerMode(1);
            sleep(5);
            PowerHal_Wrap_EnableSFLowPowerMode(0);
        }

        break;

    case 10:
        printf("Test c2ps power hal command\n");
        c2ps_hdl1 = perfLockAcq(0, 1000, perf_lock_rsc9, 2);
        printf("perfLockAcq hdl:%d\n", c2ps_hdl1);
        c2ps_hdl2 = perfLockAcq(0, 1000, perf_lock_rsc10, 2);
        printf("perfLockAcq hdl:%d\n", c2ps_hdl2);
        c2ps_hdl3 = perfLockAcq(0, 1000, perf_lock_rsc11, 2);
        printf("perfLockAcq hdl:%d\n", c2ps_hdl3);
        c2ps_hdl4 = perfLockAcq(0, 1000, perf_lock_rsc12, 2);
        printf("perfLockAcq hdl:%d\n", c2ps_hdl4);
        c2ps_hdl5 = perfLockAcq(0, 1000, perf_lock_rsc13, 2);
        printf("perfLockAcq hdl:%d\n", c2ps_hdl5);
        break;

    default:
        break;
    }
}

static void perf_msg_test(int cmd)
{
    printf("perf_msg_test cmd:%d\n", cmd);

#if 0
    int hdl1 = 0;
    int perf_lock_rsc1[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 3000000, PERF_RES_DRAM_OPP_MIN, 0};

    struct tPowerData vPowerData;
    struct tPowerData vRspData;
    struct tCusLockData vCusLockData;
    struct tPerfLockData vPerfLockData;
    struct tLockHdlData vLockHdlData;
    nsecs_t start, end;
    int interval, interval_ms;
    int i;

    switch(cmd) {
    case 1:
        vPerfLockData.hdl = 0;
        vPerfLockData.duration = 5000;
        vPerfLockData.size = 6;
        memcpy(vPerfLockData.rscList, perf_lock_rsc1, sizeof(int)*6);
        vPerfLockData.reserved = 1234;
        vPerfLockData.pid = 234;
        vPerfLockData.uid = 567;
        vPowerData.msg  = POWER_MSG_PERF_LOCK_ACQ;
        vPowerData.pBuf = (void*)&vPerfLockData;

        vRspData.pBuf = (void*)&vPerfLockData;

        start = systemTime();
        power_msg_perf_lock_acq(&vPowerData, (void **) &vRspData);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);

        printf("hdl:%d\n", vPerfLockData.hdl);
        printf("intervall:%d, %d\n", interval, interval_ms);
        break;

    case 2:
        vLockHdlData.hdl = 357;
        vLockHdlData.reserved = 678;
        vPowerData.msg  = POWER_MSG_PERF_LOCK_REL;
        vPowerData.pBuf = (void*)&vLockHdlData;
        vRspData.pBuf = (void*)&vLockHdlData;

        start = systemTime();
        power_msg_perf_lock_rel(&vPowerData, (void *) &vRspData);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("intervall:%d, %d\n", interval, interval_ms);
        break;

    case 3:
        vCusLockData.hdl = 0;
        vCusLockData.duration = 4000;
        vCusLockData.hint = 1;
        vCusLockData.pid = 123;

        vPowerData.msg  = POWER_MSG_CUS_LOCK_HINT;
        vPowerData.pBuf = (void*)&vCusLockData;

        vRspData.pBuf = (void*)&vCusLockData;

        start = systemTime();
        power_msg_cus_lock_hint(&vPowerData, (void **) &vRspData);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);

        hdl1 =  vCusLockData.hdl;
        printf("hdl1:%d\n", hdl1);
        printf("intervall:%d, %d\n", interval, interval_ms);
        break;

    case 4:
        start = systemTime();
        hdl1 = test_perfLockAcquire(0, 3000, perf_lock_rsc1, 6);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("hdl:%d\n", hdl1);
        printf("intervall:%d, %d\n", interval, interval_ms);
        break;

    case 5:
        start = systemTime();
        test_perfLockRelease(1234);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("intervall:%d, %d\n", interval, interval_ms);
        break;

    case 6:
        start = systemTime();
        hdl1 = test_perfCusLockHint(1, 3000);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("hdl:%d\n", hdl1);
        printf("intervall:%d, %d\n", interval, interval_ms);
        break;

    case 7:
        printf("test 1\n");
        for(i=0; i<300; i++) {
            hdl1 = test_perfLockAcquire(0, 3000, perf_lock_rsc1, 6);
            sleep(1);
            test_perfLockRelease(hdl1);
            sleep(1);
        }

        printf("test 2\n");
        for(i=0; i<300; i++) {
            hdl1 = test_perfCusLockHint(1, 1000);
            sleep(1);
            test_perfLockRelease(hdl1);
            sleep(1);
        }

        printf("test done\n");
        sleep(600);
        break;
    default:
        break;
    }
#endif
}

static void mode_test(int mode, int duration, int pid) {
    void *mSoHandle = NULL;
    void *func = NULL;
    int (*PowerHal_Wrap_EnableGameMode)(int32_t, int32_t) = NULL;
    int (*PowerHal_Wrap_EnableVideoMode)(int32_t, int32_t) = NULL;
    int (*PowerHal_Wrap_EnableCameraMode)(int32_t, int32_t) = NULL;
    typedef int (*enableGameMode)(int32_t, int32_t);
    typedef int (*enableVideoMode)(int32_t, int32_t);
    typedef int (*enableCameraMode)(int32_t, int32_t);

    mSoHandle = dlopen("libpowerhalwrap_vendor.so", RTLD_NOW);

    switch(mode) {
        case 1:
            if (mSoHandle != NULL) {
                func = dlsym(mSoHandle, "PowerHal_Wrap_EnableGameMode");
                PowerHal_Wrap_EnableGameMode = reinterpret_cast<enableGameMode>(func);
                if (PowerHal_Wrap_EnableGameMode == NULL) {
                    printf("load PowerHal_Wrap_EnableGameMode fail!");
                    dlclose(mSoHandle);
                }
            } else {
                printf("libpowerhalwrap dlerror:%s", dlerror());
            }

            printf("Enable / Disable PowerHal_Wrap_EnableGameMode \n");

            if (PowerHal_Wrap_EnableGameMode) {
                printf("Enable game mode pid: %d \n", pid);
                PowerHal_Wrap_EnableGameMode(1, pid);
                sleep(duration);
                printf("Disable game mode pid: %d \n", pid);
                PowerHal_Wrap_EnableGameMode(0, pid);
            }

            break;

        case 3:
            if (mSoHandle != NULL) {
                func = dlsym(mSoHandle, "PowerHal_Wrap_EnableCameraMode");
                PowerHal_Wrap_EnableCameraMode = reinterpret_cast<enableCameraMode>(func);
                if (PowerHal_Wrap_EnableCameraMode == NULL) {
                    printf("load PowerHal_Wrap_EnableCameraMode fail!");
                    dlclose(mSoHandle);
                }
            } else {
                printf("libpowerhalwrap dlerror:%s", dlerror());
            }

            printf("Enable / Disable PowerHal_Wrap_EnableCameraMode \n");

            if (PowerHal_Wrap_EnableCameraMode) {
                printf("Enable camera mode pid: %d \n", pid);
                PowerHal_Wrap_EnableCameraMode(1, pid);
                sleep(duration);
                printf("Disable camera mode pid: %d \n", pid);
                PowerHal_Wrap_EnableCameraMode(0, pid);
            }

            break;

        default:
            break;
    }

    if(mSoHandle != NULL) {
        dlclose(mSoHandle);
    }
}

static void game_mode_test(int hint, int pid) {
    void *mSoHandle = NULL;
    void *func = NULL;
    int (*PowerHal_Wrap_EnableGameMode)(int32_t, int32_t) = NULL;
    typedef int (*enableGameMode)(int32_t, int32_t);

    mSoHandle = dlopen("libpowerhalwrap_vendor.so", RTLD_NOW);
    if (mSoHandle != NULL) {
        func = dlsym(mSoHandle, "PowerHal_Wrap_EnableGameMode");
        PowerHal_Wrap_EnableGameMode = reinterpret_cast<enableGameMode>(func);
        if (PowerHal_Wrap_EnableGameMode == NULL) {
            printf("load PowerHal_Wrap_EnableGameMode fail!");
            dlclose(mSoHandle);
        }
    } else {
        printf("libpowerhalwrap dlerror:%s", dlerror());
    }

    switch(hint) {
        case 1:
            printf("Enable / Disable PowerHal_Wrap_EnableGameMode \n");

            if (PowerHal_Wrap_EnableGameMode) {
                printf("Enable game mode pid: %d \n", pid);
                PowerHal_Wrap_EnableGameMode(1, pid);
                sleep(20);
                printf("Disable game mode pid: %d \n", pid);
                PowerHal_Wrap_EnableGameMode(0, pid);
            }

            break;

        case 2:
            printf("[Stress Test] PowerHal_Wrap_EnableGameMode \n");

            if (PowerHal_Wrap_EnableGameMode) {
                for(int i=0; i<20; i++){
                    PowerHal_Wrap_EnableGameMode(1, i+1);
                    printf("Enable game mode pid: %d \n", i+1);
                }
                sleep(5);
                for(int i=0; i<20; i++){
                    PowerHal_Wrap_EnableGameMode(0, i+1);
                    printf("Disable game mode pid: %d \n", i+1);
                }
            }

            break;

        case 3:
            printf("Enable PowerHal_Wrap_EnableGameMode \n (Enable -> Enable -> Disable -> sleep 20 sec -> Disable) \n");

            if (PowerHal_Wrap_EnableGameMode) {
                printf("Enable game mode pid: %d \n", 123);
                PowerHal_Wrap_EnableGameMode(1, 123);
                printf("Enable game mode pid: %d \n", 456);
                PowerHal_Wrap_EnableGameMode(1, 456);
                printf("Disable game mode pid: %d \n", 123);
                PowerHal_Wrap_EnableGameMode(0, 123);
                sleep(20);
                printf("Disable game mode pid: %d \n", 456);
                PowerHal_Wrap_EnableGameMode(0, 456);
            }

            break;

        default:
            break;
    }

    if(mSoHandle != NULL) {
        dlclose(mSoHandle);
    }
}


static int load_capability_test_file(const char* filename, char *size) {
    FILE *file;
    int row = 0;
    int idex = 0;
    int ret = 0;
    char capability[MAX_LINES][MAX_COLUMNS][20];

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file");
        return -1;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        if(sscanf(line, "%[^,],%s", capability[row][0], capability[row][1]) == 0) {
            (void) fprintf(stderr, "Failed to parse input\n");
            int ret = fclose(file);
            if(ret != 0)
                printf("Error close file: %d", ret);

            return -1;
        }
        row++;
    }

    for (int i = 0; i < row; i++) {
           perf_lock_cap_rsc[idex] =  strtoul(capability[i][0], NULL, 16);
           idex++;
           perf_lock_cap_rsc[idex] =  atoi(capability[i][1]);
           idex++;
    }
    *size = idex;

    ret = fclose(file);
    if(ret != 0) {
        printf("Error close file: %d", ret);
        return -1;
    }

    return 0;
}

static void sport_mode_test(int enable) {
    char size = '\0';
    static int open_flag = 0;

    printf("sport_mode_test %d\n", enable);

    if (open_flag == 0) {
        if(load_perf_api()!=0) {
            printf("load perf api fail\n");
            return;
        }

        if ( load_capability_test_file(CAPABILITY_FILE_PATH, &size) ) {
            printf("no capability_test_file\n");
            return;
        }
         open_flag = 1;
    }

    if (enable != 0) {
            for (int i = 0; i < size; i++) {
                printf("perf_lock_cap_rsc[%d] = %x\n", i, perf_lock_cap_rsc[i]);
            }
            cap_hdl = perfLockAcq(cap_hdl, 30000, perf_lock_cap_rsc, size);
            printf("perfLockAcq hdl:%d size = %d \n", cap_hdl, size);
    } else {
        if ( cap_hdl != 0 ) {
            printf("sport_mode_test perfLockRel cap_hdl %d\n", cap_hdl);
            perfLockRel(cap_hdl);
            cap_hdl = 0;
        }
    } 
}

static void usage(char *cmd);

int main(int argc, char* argv[])
{
    int test_cmd=0;
    int hint=0, timeout=0, data=0, duration=0;
    int cmd=0, p1=0;
    int hdl = -1, ret;
    bool support;
    std::string data_str = "";
    std::vector<int32_t> perf_lock_rsc = {0,0};

    if(argc < 2) {
        usage(argv[0]);
        return 0;
    }

    ndk::SpAIBinder pwBinder = ndk::SpAIBinder(
        AServiceManager_getService(gPowerHal_Aidl_instance.c_str()));
    gPowerHal_Aidl_ = aidl::android::hardware::power::IPower::fromBinder(pwBinder);
    if (gPowerHal_Aidl_ == nullptr) {
        fprintf(stderr, "no IPower AIDL\n");
        return -1;
    }

    gIMtkPowerService = aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::fromBinder(SpAIBinder(AServiceManager_getService(gIMtkPowerService_instance.c_str())));
    if (gIMtkPowerService == nullptr) {
        fprintf(stderr, "cannot load IMtkPowerService \n");
        return -1;
    }

    test_cmd = atoi(argv[1]);
    //printf("argc:%d, command:%d\n", argc, command);
    switch(test_cmd) {
        case CMD_AOSP_POWER_HINT:
        case CMD_AOSP_POWER_HINT_1_1:
        case CMD_MTK_POWER_HINT:
        case CMD_CUS_POWER_HINT:
        case CMD_QUERY_INFO:
        case CMD_SET_INFO:
        case CMD_SET_INFO_ASYNC:
        case CMD_PERF_CUS_LOCK_ACQ:
        case CMD_AIDL_SET_BOOST:
        case CMD_GAME_MODE_TEST:
        case CMD_AIDL_SET_MODE:
            if(argc!=4) {
                usage(argv[0]);
                return -1;
            }
            break;

        case CMD_PERF_LOCK_REL:
        case CMD_UNIT_TEST:
        case CMD_PERF_MSG_TEST:
        case CMD_AIDL_IS_MODE_SUPPORT:
        case CMD_AIDL_IS_BOOST_SUPPORT:
            if(argc!=3) {
                usage(argv[0]);
                return -1;
            }
            break;

        case CMD_MODE_TEST:
        case CMD_PERF_LOCK_ACQ:
            if(argc!=5) {
                usage(argv[0]);
                return -1;
            }
            break;

        case CMD_SPORT_MODE_TEST:
            if (argc != 3) {
                usage(argv[0]);
                return -1;
            }
            break;

        default:
            usage(argv[0]);
            return -1;
    }

    if(test_cmd == CMD_AOSP_POWER_HINT || test_cmd == CMD_AOSP_POWER_HINT_1_1 || test_cmd == CMD_MTK_POWER_HINT || test_cmd == CMD_CUS_POWER_HINT  || test_cmd == CMD_GAME_MODE_TEST) {
        hint = atoi(argv[2]);
        data = atoi(argv[3]);
    } else if(test_cmd == CMD_QUERY_INFO || test_cmd == CMD_SET_INFO_ASYNC) {
        cmd = atoi(argv[2]);
        if (cmd == MTKPOWER_CMD_GET_DEBUG_DUMP_ALL || cmd == MTKPOWER_CMD_GET_RES_SETTING ||
            cmd == MTKPOWER_CMD_GET_RES_CTRL_ON || cmd == MTKPOWER_CMD_GET_RES_CTRL_OFF)
            p1 = strtol(argv[3],NULL,16);
        else
            p1 = atoi(argv[3]);
    } else if(test_cmd == CMD_SET_INFO) {
        cmd = atoi(argv[2]);
        data_str = argv[3];
    } else if(test_cmd == CMD_PERF_LOCK_ACQ) {
        timeout = atoi(argv[2]);
        perf_lock_rsc[0] = strtol(argv[3],NULL,16);
        perf_lock_rsc[1] = atoi(argv[4]);
    } else if(test_cmd == CMD_PERF_CUS_LOCK_ACQ) {
        hint = atoi(argv[2]);
        timeout = atoi(argv[3]);
    } else if(test_cmd == CMD_PERF_LOCK_REL) {
        hdl = atoi(argv[2]);
    } else if(test_cmd == CMD_UNIT_TEST || test_cmd == CMD_PERF_MSG_TEST) {
        cmd = atoi(argv[2]);
    } else if(test_cmd == CMD_AIDL_SET_MODE || test_cmd == CMD_AIDL_SET_BOOST) {
        hint = atoi(argv[2]);
        cmd = atoi(argv[3]);
    } else if(test_cmd == CMD_AIDL_IS_MODE_SUPPORT || test_cmd == CMD_AIDL_IS_BOOST_SUPPORT) {
        hint = atoi(argv[2]);
    } else if(test_cmd == CMD_MODE_TEST) {
        hint = atoi(argv[2]);
        duration = atoi(argv[3]);
        data = atoi(argv[4]);
    }

    /* command */
    if(test_cmd == CMD_AOSP_POWER_HINT) {
        fprintf(stderr, "no IPower\n");
        return -1;
    } else if(test_cmd == CMD_AOSP_POWER_HINT_1_1) {
        fprintf(stderr, "no IPower 1.1\n");
        return -1;
    } else if(test_cmd == CMD_MTK_POWER_HINT) {
        ndk::ScopedAStatus ret = gIMtkPowerService->mtkPowerHint(hint, data);
        sleep(3);
    } else if(test_cmd == CMD_CUS_POWER_HINT) {
        ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(hint, data);
        sleep(3);
    } else if(test_cmd == CMD_QUERY_INFO) {
        ndk::ScopedAStatus ret = gIMtkPowerService->querySysInfo(cmd, p1, &data);
        printf("data:%d\n", data);
    } else if(test_cmd == CMD_SET_INFO) {
        ndk::ScopedAStatus _ret = gIMtkPowerService->setSysInfo(cmd, data_str, &ret);
        printf("ret:%d\n", ret);
    } else if(test_cmd == CMD_SET_INFO_ASYNC) {
        std::string str = "test_async";
        ndk::ScopedAStatus ret = gIMtkPowerService->setSysInfoAsync(cmd, str);
    } else if(test_cmd == CMD_PERF_LOCK_ACQ) {
        ndk::ScopedAStatus ret = gIMtkPowerService->perfLockAcquire(0, timeout, perf_lock_rsc, getpid(), gettid(), &hdl);
        printf("hdl:%d\n", hdl);
    } else if(test_cmd == CMD_PERF_CUS_LOCK_ACQ) {
        ndk::ScopedAStatus ret = gIMtkPowerService->perfCusLockHint(hint, timeout, getpid(), &hdl);
        printf("hdl:%d\n", hdl);
    } else if(test_cmd == CMD_PERF_LOCK_REL) {
        ndk::ScopedAStatus ret = gIMtkPowerService->perfLockRelease(hdl, 0);
    } else if(test_cmd == CMD_UNIT_TEST) {
        unit_test(cmd);
    } else if(test_cmd == CMD_PERF_MSG_TEST) {
        perf_msg_test(cmd);
    } else if(test_cmd == CMD_AIDL_SET_MODE) {
        if(cmd)
            gPowerHal_Aidl_->setMode((Mode)hint, true);
        else
            gPowerHal_Aidl_->setMode((Mode)hint, false);
    } else if(test_cmd == CMD_AIDL_IS_MODE_SUPPORT) {
        gPowerHal_Aidl_->isModeSupported((Mode)hint, &support);
        printf("support:%d\n", support);
    } else if(test_cmd == CMD_AIDL_SET_BOOST) {
        gPowerHal_Aidl_->setBoost((Boost)hint, cmd);
    } else if(test_cmd == CMD_AIDL_IS_BOOST_SUPPORT) {
        gPowerHal_Aidl_->isBoostSupported((Boost)hint, &support);
        printf("support:%d\n", support);
    } else if(test_cmd == CMD_MODE_TEST) {
        mode_test(hint, duration, data);
    } else if(test_cmd == CMD_GAME_MODE_TEST) {
        game_mode_test(hint, data);
    } else if (test_cmd == CMD_SPORT_MODE_TEST) {
        sport_mode_test(atoi(argv[2]));
    }

    if(lib_handle) {
        dlclose(lib_handle);
    }

    return 0;
}

static void usage(char *cmd) {
    fprintf(stderr, "\nUsage: %s command scenario\n"
                    "    command\n"
                    "        1: AOSP power hint\n"
                    "        2: AOSP power 1.1 hint\n"
                    "        3: MTK power hint\n"
                    "        4: MTK cus power hint\n"
                    "        5: query info\n"
                    "        6: set info\n"
                    "        7: set info async\n"
                    "        8: perf lock acquire(duration, cmd, param)\n"
                    "        9: perf cus lock(hint, duration)\n"
                    "        10: perf lock release\n"
                    "        11: unit test\n"
                    "        12: perf msg test\n"
                    "        13: AIDL set mode\n"
                    "        14: AIDL is mode support\n"
                    "        15: AIDL set boost\n"
                    "        16: AIDL is boost support\n"
                    "        17: mode test(mode, duration, pid)\n"
                    "        18: Game mode test(case, pid)\n"
                    "        19: Sport mode test(id, 1: start 0: end) \n", cmd);

}

//} // namespace

