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

#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPerf.h>
#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPower.h>

#include "mtkperf_resource.h"
#include "mtkpower_types.h"
#include <utils/Timers.h>

using namespace vendor::mediatek::hardware::mtkpower::V1_0;
using ::android::hardware::hidl_string;

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
    CMD_PERF_LOCK_REL,
    CMD_UNIT_TEST,
    CMD_UPDATE_TEST,
    CMD_VPU_UNIT_TEST,
    CMD_CPU_UNIT_TEST,
    CMD_CFP_UNIT_TEST,
    CMD_GPU_UNIT_TEST,
};

#if 0
mtkPowerHint(MtkPowerHint hint, int32_t data);
scnReg() generates (int32_t hdl);
scnConfig(int32_t hdl, MtkPowerCmd cmd, int32_t param1, int32_t param2, int32_t param3, int32_t param4);
scnUnreg(int32_t hdl);
scnEnable(int32_t hdl, int32_t timeout);
scnDisable(int32_t hdl);
#endif

#define PERF_LOCK_LIB_FULL_NAME  "libmtkperf_client_vendor.so"

typedef int (*perf_lock_acq)(int, int, int[], int);
typedef int (*perf_lock_rel)(int);

/* function pointer to perfserv client */
static int  (*perfLockAcq)(int, int, int[], int) = NULL;
static int  (*perfLockRel)(int) = NULL;

void *lib_handle = NULL;


static int load_perf_api(void)
{
    void *func = NULL;

    lib_handle = dlopen(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);

    if (lib_handle == NULL) {
        printf("dlopen fail: %s\n", dlerror());
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_acq");
    perfLockAcq = reinterpret_cast<perf_lock_acq>(func);

    if (perfLockAcq == NULL) {
        printf("perfLockAcq error: %s\n", dlerror());
        dlclose(lib_handle);
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_rel");
    perfLockRel = reinterpret_cast<perf_lock_rel>(func);

    if (perfLockRel == NULL) {
        printf("perfLockRel error: %s\n", dlerror());
        dlclose(lib_handle);
        return -1;
    }

    return 0;
}

static void unit_test(int cmd)
{
    int perf_lock_rsc1[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 3000000, PERF_RES_DRAM_OPP_MIN, 0};
    int perf_lock_rsc2[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 3000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 1000000, PERF_RES_DRAM_OPP_MIN, 0};
    int perf_lock_rsc3[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 3000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 1000000, PERF_RES_DRAM_OPP_MIN, 0, PERF_RES_SCHED_BOOST_VALUE_TA, 100};
    int perf_lock_rsc4[] = {PERF_RES_THERMAL_POLICY, 8, PERF_RES_NET_WIFI_LOW_LATENCY, 1, PERF_RES_NET_NETD_BOOST_UID, 5};
    int perf_lock_rsc5[] = {PERF_RES_THERMAL_POLICY, 5, PERF_RES_NET_WIFI_LOW_LATENCY, 0, PERF_RES_NET_NETD_BOOST_UID, 3};
    int perf_lock_rsc6[] = {PERF_RES_THERMAL_POLICY, 18, PERF_RES_NET_WIFI_LOW_LATENCY, 1, PERF_RES_NET_NETD_BOOST_UID, 8};
    int perf_lock_rsc7[] = {PERF_RES_NET_WIFI_LOW_LATENCY, 1};
    int perf_lock_rsc8[] = {PERF_RES_NET_WIFI_LOW_LATENCY, 1, PERF_RES_CPUFREQ_MIN_CLUSTER_0};
    int perf_lock_rsc9[] = {PERF_RES_NET_NETD_BOOST_UID, 1234};
    int perf_lock_rsc10[] = {PERF_RES_NET_NETD_BOOST_UID, 1};
    int perf_lock_rsc11[] = {PERF_RES_NET_MD_LOW_LATENCY, 1, PERF_RES_UX_PREDICT_LOW_LATENCY, 1};
    int perf_lock_rsc12[] = {PERF_RES_NET_MD_CERT_PID, 1234};
    int perf_lock_rsc13[] = {PERF_RES_NET_MD_WEAK_SIG_OPT, 1};
    int perf_lock_rsc14[] = {PERF_RES_SCHED_CPU_PREFER_TASK_1_BIG, 1};
    int perf_lock_rsc15[] = {PERF_RES_DRAM_CM_RATIO_UP_X_3, 30, PERF_RES_DRAM_CM_RATIO_UP_X_4, 30};
    int perf_lock_rsc_pro_1[] = {PERF_RES_CPUFREQ_PERF_MODE, 1};
    int perf_lock_rsc_pro_2[] = {PERF_RES_THERMAL_POLICY, 8};
    int perf_lock_rsc_ril[] = {PERF_RES_NET_MD_WEAK_SIG_OPT, 1};

    int hdl1 = 0, hdl2 = 0, hdl3 = 0;
    int i;
    nsecs_t start, end;
    int interval, interval_ms;

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

        for(i=0; i<300; i++) {
            hdl1 = perfLockAcq(0, 120000, perf_lock_rsc1, 2);
            printf("perfLockAcq hdl:%d\n", hdl1);
            usleep(300000);
            //perfLockRel(hdl1);
        }
        sleep(180);
        for(i=0; i<300; i++) {
            hdl1 = perfLockAcq(0, 120000, perf_lock_rsc1, 2);
            printf("perfLockAcq hdl:%d\n", hdl1);
            usleep(300000);
            //perfLockRel(hdl1);
        }
        sleep(180);
        break;

    case 4:

        hdl1 = perfLockAcq(0, 4000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 4000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(3);
        perfLockRel(hdl1);
        sleep(10);
        break;

    case 5:

        hdl1 = perfLockAcq(0, 4000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(10);
        hdl1 = perfLockAcq(hdl1, 4000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(10);
        break;

    case 6:

        hdl1 = perfLockAcq(0, 4000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 4000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(10);
        break;

    case 7:

        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc1, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(2);
        hdl2 = perfLockAcq(0, 10000, perf_lock_rsc2, 6);
        printf("perfLockAcq hdl:%d\n", hdl2);
        sleep(2);
        perfLockRel(hdl2);
        sleep(2);
        perfLockRel(hdl1);
        break;

    case 8:

        hdl1 = perfLockAcq(0, 1000, perf_lock_rsc3, 2);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(2);
        hdl1 = perfLockAcq(0, 1000, perf_lock_rsc3, 4);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(2);
        hdl1 = perfLockAcq(0, 1000, perf_lock_rsc3, 6);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(2);
        hdl1 = perfLockAcq(0, 1000, perf_lock_rsc3, 8);
        printf("perfLockAcq hdl:%d\n", hdl1);
        sleep(2);
        break;

    case 9:
        hdl1 = perfLockAcq(0, 20000, perf_lock_rsc4, 6);
        printf("perfLockAcq hdl1:%d\n", hdl1);
        sleep(2);
        hdl2 = perfLockAcq(0, 20000, perf_lock_rsc5, 6);
        printf("perfLockAcq hdl2:%d\n", hdl2);
        sleep(2);
        hdl3 = perfLockAcq(0, 20000, perf_lock_rsc6, 6);
        printf("perfLockAcq hdl3:%d\n", hdl3);
        sleep(2);
        perfLockRel(hdl1);
        sleep(2);
        perfLockRel(hdl2);
        sleep(2);
        perfLockRel(hdl3);
        break;

    /* wifi low latency */
    case 10:
        hdl1 = perfLockAcq(0, 18000, perf_lock_rsc7, 2);
        printf("perfLockAcq hdl1:%d\n", hdl1);
        sleep(20);
        break;

    /* wrong data size */
    case 11:
        hdl1 = perfLockAcq(0, 18000, perf_lock_rsc8, 3);
        printf("perfLockAcq hdl1:%d\n", hdl1);
        sleep(2);
        break;

    /* infinite duration */
    case 12:
        hdl1 = perfLockAcq(0, 0, perf_lock_rsc1, 4);
        printf("perfLockAcq hdl1:%d\n", hdl1);
        sleep(5);
        perfLockRel(hdl1);
        break;

    case 13:
        hdl1 = perfLockAcq(0, 3000, perf_lock_rsc1, 4);
        sleep(5);
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc1, 4);
        sleep(5);
        hdl1 = perfLockAcq(hdl1, 10000, perf_lock_rsc2, 4);
        sleep(5);
        perfLockRel(hdl1);
        break;

    /* netd */
    case 14:
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc9, 2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc10, 2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        break;

    /* net */
    case 15:
        hdl1 = perfLockAcq(0, 20000, perf_lock_rsc11, 4);
        sleep(18);
        hdl1 = perfLockAcq(hdl1, 20000, perf_lock_rsc11, 4);
        sleep(18);
        hdl1 = perfLockAcq(hdl1, 20000, perf_lock_rsc11, 4);
        sleep(18);
        break;

    case 16:
        hdl1 = perfLockAcq(0, 120000, perf_lock_rsc11, 4);
        sleep(125);
        break;

    case 17:
        hdl1 = perfLockAcq(0, 25000, perf_lock_rsc12, 2);
        sleep(28);
        break;

    case 18:
        hdl1 = perfLockAcq(0, 60000, perf_lock_rsc12, 2);
        sleep(63);
        break;

    case 19:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc13, 2);
        sleep(33);
        break;

    case 20:
        perf_lock_rsc14[1] = (int)gettid();
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc14, 2);
        sleep(5);
        perf_lock_rsc14[0] = PERF_RES_SCHED_CPU_PREFER_TASK_1_LITTLE;
        hdl1 = perfLockAcq(hdl1, 10000, perf_lock_rsc14, 2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(1);
        break;

    case 21:
        hdl1 = perfLockAcq(0, 0, perf_lock_rsc15, 4);
        sleep(10);
        perfLockRel(hdl1);
        break;

    case 22:
        start = systemTime();
        hdl1 = perfLockAcq(0, 2000, perf_lock_rsc_pro_1, 2);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("hdl:%d\n", hdl1);
        printf("pro 1 intervall:%d, %d\n", interval, interval_ms);
        sleep(10);
        break;

    case 23:
        start = systemTime();
        hdl1 = perfLockAcq(0, 2000, perf_lock_rsc_pro_2, 2);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("hdl:%d\n", hdl1);
        printf("pro 2 intervall:%d, %d\n", interval, interval_ms);
        sleep(10);
        break;

    case 24:
        start = systemTime();
        hdl1 = perfLockRel(12345);
        end = systemTime();
        interval = (int)(end - start);
        interval_ms = ns2ms(end - start);
        printf("lock rel intervall:%d, %d\n", interval, interval_ms);
        sleep(10);
        break;

    case 25:
        hdl1 = perfLockAcq(0, 5000, perf_lock_rsc4, 4);
        hdl2 = perfLockAcq(0, 5000, perf_lock_rsc4, 4);
        hdl3 = perfLockAcq(0, 5000, perf_lock_rsc4, 4);
        sleep(30);
        break;

    case 26:
        hdl1 = perfLockAcq(0, 0, perf_lock_rsc1, 6);
        perfLockRel(hdl1);
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc1, 6);
        perfLockRel(hdl1);
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc1, 6);
        perfLockRel(hdl1);
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc1, 6);
        perfLockRel(hdl1);
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc1, 6);
        perfLockRel(hdl1);
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc1, 6);
        perfLockRel(hdl1);
        sleep(60);
        break;

    case 27:
        hdl1 = 0;
        for(i=0; i<20000; i++) {
            hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc1, 6);
            perfLockRel(hdl1);
            if (i != 0 && i % 2000 == 0) {
                sleep(10);
                printf("%d\n", i);
            }
        }
        sleep(30);
        break;

    case 28:
        hdl1 = 0;

        /* normal */
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc_ril, 2);
        sleep(3);
        perfLockRel(hdl1);
        sleep(10);

        /* lock acq */
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc_ril, 2);
        sleep(3);
        perfLockRel(hdl1);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 0, perf_lock_rsc_ril, 2);
        sleep(3);
        perfLockRel(hdl1);
        sleep(10);
        break;

    default:
        break;
    }
}

static void update_test(int cmd)
{
    int perf_lock_rsc1[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000};
    int perf_lock_rsc2[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1400000};
    int perf_lock_rsc3[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1400000, PERF_RES_CPUFREQ_MAX_CLUSTER_0, 1200000};
    int perf_lock_rsc4[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MAX_CLUSTER_0, 1200000};
    int perf_lock_rsc5[] = {PERF_RES_GPU_FREQ_MAX, 2};
    int perf_lock_rsc6[] = {PERF_RES_GPU_FREQ_MAX, 2, PERF_RES_GPU_FREQ_MIN, 1};
    int perf_lock_rsc7[] = {PERF_RES_GPU_FREQ_MAX, 1};
    int perf_lock_rsc8[] = {PERF_RES_DRAM_OPP_MIN, 0, PERF_RES_GPU_FREQ_MAX, 1, PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1400000, PERF_RES_CPUFREQ_MAX_CLUSTER_1, 1200000};
    int perf_lock_rsc9[] = {PERF_RES_THERMAL_POLICY, 3};
    int perf_lock_rsc10[] = {PERF_RES_THERMAL_POLICY, 8};
    int perf_lock_rsc11[] = {PERF_RES_NET_NETD_BOOST_UID, 123};
    int perf_lock_rsc12[] = {PERF_RES_NET_NETD_BOOST_UID, 456};
    int perf_lock_rsc13[] = {PERF_RES_DRAM_CM_MGR_CAM_ENABLE, 1, PERF_RES_SCHED_PREFER_IDLE_TA, 1, PERF_RES_SCHED_PREFER_IDLE_FG, 1};
    int perf_lock_rsc14[] = {PERF_RES_CPUFREQ_PERF_MODE, 1};
    int perf_lock_rsc15[] = {PERF_RES_NET_MD_LOW_LATENCY, 1};

    int hdl1 = 0;

    if(load_perf_api()!=0) {
        fprintf(stderr, "dlopen fail\n");
        return;
    }

    switch(cmd) {
    case 1:
        hdl1 = perfLockAcq(0, 5000, perf_lock_rsc1, 2);
        sleep(8);

        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc1, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc1, 2);
        sleep(8);

        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc1, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc2, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc3, 4);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc4, 4);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc5, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc6, 4);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc7, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc8, 8);
        sleep(3);

        break;

    case 2:
        hdl1 = perfLockAcq(0, 5000, perf_lock_rsc9, 2);
        sleep(8);

        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc9, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc10, 2);
        sleep(8);

        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc9, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc10, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc11, 2);
        sleep(3);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc12, 2);
        sleep(3);
        break;

    case 3:
        hdl1 = perfLockAcq(0, 5000, perf_lock_rsc13, 6);
        sleep(2);
        hdl1 = perfLockAcq(hdl1, 5000, perf_lock_rsc14, 2);
        sleep(6);
        break;

    case 4:
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc15, 2);
        sleep(8);
        perfLockAcq(hdl1, 10000, perf_lock_rsc15, 2);
        sleep(8);
        perfLockAcq(hdl1, 10000, perf_lock_rsc15, 2);
        sleep(8);
        perfLockAcq(hdl1, 10000, perf_lock_rsc15, 2);
        sleep(15);
        break;

    default:
        break;
    }
}

static void unit_vpu_test(int cmd)
{
    int perf_lock_rsc1[] = {PERF_RES_AI_VPU_FREQ_MIN_CORE_0, 20, PERF_RES_AI_VPU_FREQ_MAX_CORE_0, 80,
                            PERF_RES_AI_VPU_FREQ_MIN_CORE_1, 50, PERF_RES_AI_VPU_FREQ_MAX_CORE_1, 70};
    int perf_lock_rsc2[] = {PERF_RES_AI_VPU_FREQ_MIN_CORE_0, 50, PERF_RES_AI_VPU_FREQ_MAX_CORE_0, 50,
                            PERF_RES_AI_VPU_FREQ_MIN_CORE_1, 30, PERF_RES_AI_VPU_FREQ_MAX_CORE_1, 40};
    int perf_lock_rsc3[] = {PERF_RES_AI_MDLA_FREQ_MIN, 7, PERF_RES_AI_MDLA_FREQ_MAX, 93};
    int hdl1, hdl2 = 0;

    if(load_perf_api()!=0) {
        fprintf(stderr, "dlopen fail\n");
        return;
    }

    switch(cmd) {
    case 1:
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc1, 8);
        sleep(12);
        break;

    case 2:
        hdl2 = perfLockAcq(0, 10000, perf_lock_rsc2, 8);
        sleep(12);
        break;

    case 3:
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc1, 8);
        hdl2 = perfLockAcq(0, 10000, perf_lock_rsc2, 8);
        sleep(2);
        perfLockRel(hdl2);
        sleep(2);
        perfLockRel(hdl1);
        break;

    case 4:
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc1, 8);
        hdl2 = perfLockAcq(0, 10000, perf_lock_rsc2, 8);
        sleep(2);
        perfLockRel(hdl1);
        sleep(2);
        perfLockRel(hdl2);
        break;

    case 5:
        hdl1 = perfLockAcq(0, 10000, perf_lock_rsc3, 4);
        sleep(12);
        break;

    default:
        break;
    }
}

static void unit_cpu_test(int cmd)
{
    int perf_lock_rsc1[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 1200000};
    int perf_lock_rsc2[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1400000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 1600000};
    int perf_lock_rsc3[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MAX_CLUSTER_0, 1400000};
    int perf_lock_rsc4[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 1600000};
    int perf_lock_rsc5[] = {PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1, 1200000};
    int perf_lock_rsc6[] = {PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0, 1400000, PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1, 1600000};
    int perf_lock_rsc7[] = {PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0, 1000000, PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0, 1400000};
    int perf_lock_rsc8[] = {PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0, 1600000};
    int perf_lock_rsc9[] = {PERF_RES_CPUFREQ_MIN_CLUSTER_0, 3000000, PERF_RES_CPUFREQ_MIN_CLUSTER_1, 3000000, PERF_RES_POWERHAL_SCREEN_OFF_STATE, MTKPOWER_SCREEN_OFF_ALWAYS_VALID};

    int hdl1 = 0, hdl2 = 0;

    if(load_perf_api()!=0) {
        fprintf(stderr, "dlopen fail\n");
        return;
    }

    switch(cmd) {
    case 1:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc1, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc2, 4);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        break;

    case 2:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc1, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc2, 4);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        break;

    case 3:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc3, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc4, 2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        break;

    case 4:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc3, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc4, 2);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        break;

    case 5:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc5, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc6, 4);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        break;

    case 6:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc5, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc6, 4);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        break;

    case 7:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc7, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc8, 2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        break;

    case 8:
        hdl1 = perfLockAcq(0, 30000, perf_lock_rsc7, 4);
        sleep(5);
        hdl2 = perfLockAcq(0, 30000, perf_lock_rsc8, 2);
        sleep(5);
        perfLockRel(hdl2);
        sleep(5);
        perfLockRel(hdl1);
        sleep(5);
        break;

    case 9:
        perfLockAcq(0, 360000, perf_lock_rsc9, 4);
        break;

    case 10:
        perfLockAcq(0, 360000, perf_lock_rsc9, 6);
        break;

    default:
        break;
    }
}

static void unit_test_gpu(void)
{
    int perf_lock_rsc1[] = {PERF_RES_GPU_FREQ_MIN, 0};
    int perf_lock_rsc2[] = {PERF_RES_GPU_FREQ_MAX, 1};

    int hdl1 = -1, hdl2 = -1;

    if(load_perf_api()!=0) {
        fprintf(stderr, "dlopen fail\n");
        return;
    }

    /* case 1 */
    // min opp:0
    hdl1 = perfLockAcq(0, 20000, perf_lock_rsc1, 2);
    sleep(5);
    perfLockRel(hdl1);
    sleep(5);

    /* case 2 */
    // max opp:1
    hdl2 = perfLockAcq(0, 20000, perf_lock_rsc2, 2);
    sleep(5);
    perfLockRel(hdl2);
    sleep(5);

    /* case 3 */
    // min:0 -> max:0 -> max:1 -> max:-1
    hdl1 = perfLockAcq(0, 20000, perf_lock_rsc1, 2);
    sleep(5);
    hdl2 = perfLockAcq(0, 20000, perf_lock_rsc2, 2);
    sleep(5);
    perfLockRel(hdl1);
    sleep(5);
    perfLockRel(hdl2);
    sleep(5);

    /* case 4 */
    // min:0 -> max:0 -> max:-1 -> min:-1
    hdl1 = perfLockAcq(0, 20000, perf_lock_rsc1, 2);
    sleep(5);
    hdl2 = perfLockAcq(0, 20000, perf_lock_rsc2, 2);
    sleep(5);
    perfLockRel(hdl2);
    sleep(5);
    perfLockRel(hdl1);
    sleep(5);

    return;
}


static void usage(char *cmd);

int main(int argc, char* argv[])
{
    int test_cmd=0;
    int hint=0, timeout=0, data=0;
    int cmd=0, p1=0;
    int hdl = -1;
    hidl_string data_str;
    android::hardware::hidl_vec<int32_t> perf_lock_rsc = {0,0};

    android::sp<IMtkPower> gMtkPower = nullptr;
    android::sp<IMtkPerf> gMtkPerf = nullptr;

    if(argc < 2) {
        usage(argv[0]);
        return 0;
    }

    //fprintf(stderr, "IMtkPower\n");
    //sleep(1);
    gMtkPower = IMtkPower::tryGetService();
    if (gMtkPower == nullptr) {
        fprintf(stderr, "no IMtkPower\n");
        return -1;
    }
    //fprintf(stderr, "IMtkPerf\n");
    //sleep(1);
    gMtkPerf = IMtkPerf::tryGetService();
    if (gMtkPerf == nullptr) {
        fprintf(stderr, "no IMtkPerf\n");
        return -1;
    }


    test_cmd = atoi(argv[1]);
    //printf("argc:%d, command:%d\n", argc, command);
    switch(test_cmd) {
        case CMD_GPU_UNIT_TEST:
            if(argc!=2) {
                usage(argv[0]);
                return -1;
            }
            break;

        case CMD_AOSP_POWER_HINT:
        case CMD_AOSP_POWER_HINT_1_1:
        case CMD_MTK_POWER_HINT:
        case CMD_CUS_POWER_HINT:
        case CMD_QUERY_INFO:
        case CMD_SET_INFO:
        case CMD_SET_INFO_ASYNC:
            if(argc!=4) {
                usage(argv[0]);
                return -1;
            }
            break;

        case CMD_PERF_LOCK_REL:
        case CMD_UNIT_TEST:
        case CMD_UPDATE_TEST:
        case CMD_VPU_UNIT_TEST:
        case CMD_CPU_UNIT_TEST:
        case CMD_CFP_UNIT_TEST:
            if(argc!=3) {
                usage(argv[0]);
                return -1;
            }
            break;

        case CMD_PERF_LOCK_ACQ:
            if(argc!=5) {
                usage(argv[0]);
                return -1;
            }
            break;

        default:
            usage(argv[0]);
            return -1;
    }

    if(test_cmd == CMD_AOSP_POWER_HINT || test_cmd == CMD_AOSP_POWER_HINT_1_1 || test_cmd == CMD_MTK_POWER_HINT || test_cmd == CMD_CUS_POWER_HINT) {
        hint = atoi(argv[2]);
        data = atoi(argv[3]);
    }
    else if(test_cmd == CMD_QUERY_INFO || test_cmd == CMD_SET_INFO_ASYNC) {
        cmd = atoi(argv[2]);
        if (cmd == MTKPOWER_CMD_GET_DEBUG_DUMP_ALL || cmd == MTKPOWER_CMD_GET_RES_SETTING ||
            cmd == MTKPOWER_CMD_GET_RES_CTRL_ON || cmd == MTKPOWER_CMD_GET_RES_CTRL_OFF)
            p1 = strtol(argv[3],NULL,16);
        else
            p1 = atoi(argv[3]);
    }
    else if(test_cmd == CMD_SET_INFO) {
        cmd = atoi(argv[2]);
        data_str = argv[3];
    }
    else if(test_cmd == CMD_PERF_LOCK_ACQ) {
        timeout = atoi(argv[2]);
        perf_lock_rsc[0] = strtol(argv[3],NULL,16);
        perf_lock_rsc[1] = atoi(argv[4]);
    }
    else if(test_cmd == CMD_PERF_LOCK_REL) {
        hdl = atoi(argv[2]);
    }
    else if(test_cmd == CMD_UNIT_TEST || test_cmd == CMD_UPDATE_TEST || test_cmd == CMD_VPU_UNIT_TEST || test_cmd == CMD_CPU_UNIT_TEST || test_cmd == CMD_CFP_UNIT_TEST) {
        cmd = atoi(argv[2]);
    }

    /* command */
    if(test_cmd == CMD_AOSP_POWER_HINT) {
        fprintf(stderr, "no IPower\n");
        return -1;
    }
    else if(test_cmd == CMD_AOSP_POWER_HINT_1_1) {
        fprintf(stderr, "no IPower 1.1\n");
        return -1;
    }
    else if(test_cmd == CMD_MTK_POWER_HINT)
        gMtkPower->mtkPowerHint(hint, data);
    else if(test_cmd == CMD_CUS_POWER_HINT)
        gMtkPower->mtkCusPowerHint(hint, data);
    else if(test_cmd == CMD_QUERY_INFO) {
        data = gMtkPower->querySysInfo(cmd, p1);
        printf("data:%d\n", data);
    }
    else if(test_cmd == CMD_SET_INFO) {
        int ret;
        ret = gMtkPower->setSysInfo(cmd, data_str);
        printf("ret:%d\n", ret);
    }
    else if(test_cmd == CMD_SET_INFO_ASYNC) {
        hidl_string str = "test_async";
        gMtkPower->setSysInfoAsync(cmd, str);
    }
    else if(test_cmd == CMD_PERF_LOCK_ACQ) {
        hdl = gMtkPerf->perfLockAcquire(0, timeout, perf_lock_rsc, gettid());
        printf("hdl:%d\n", hdl);
    }
    else if(test_cmd == CMD_PERF_LOCK_REL) {
        gMtkPerf->perfLockRelease(hdl, 0);
    }
    else if(test_cmd == CMD_UNIT_TEST) {
        unit_test(cmd);
    }
    else if(test_cmd == CMD_UPDATE_TEST) {
        update_test(cmd);
    }
    else if(test_cmd == CMD_VPU_UNIT_TEST) {
        unit_vpu_test(cmd);
    }
    else if(test_cmd == CMD_CPU_UNIT_TEST) {
        unit_cpu_test(cmd);
    }
    else if(test_cmd == CMD_GPU_UNIT_TEST) {
        unit_test_gpu();
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
                    "        9: perf lock release\n"
                    "        10: unit test\n"
                    "        11: update test\n"
                    "        12: vpu test\n"
                    "        13: cpu test\n"
                    "        14: cfp test\n"
                    "        15: gpu unit test\n", cmd);
}



//} // namespace

