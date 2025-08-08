//#ifdef __cplusplus
//extern "C" {
//#endif

/*** STANDARD INCLUDES *******************************************************/
#define LOG_TAG "mtkpower@impl"

#include <log/log.h>
#include <dlfcn.h>
#include "libpowerhal_wrap.h"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

#define LIB_FULL_NAME "libpowerhal.so"

typedef int (*libpowerhal_notify_state)(const char*, const char*, int, int, int);
typedef int (*libpowerhal_user_scn_disable_all)(void);
typedef int (*libpowerhal_user_scn_restore_all)(void);
typedef int (*libpowerhal_set_sysInfo)(int, const char*);
typedef int (*libpowerhal_user_get_capability)(int, int);
typedef int (*libpowerhal_lock_acq)(int*, int, int, int, int, int);
typedef int (*libpowerhal_cus_lock_hint)(int, int, int);
typedef int (*libpowerhal_cus_lock_hint_data)(int, int, int, int*, int);
typedef int (*libpowerhal_lock_rel)(int);
typedef int (*libpowerhal_init)(int);
typedef int (*libpowerhal_set_ScnCallback)(int, const std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback> &);
//SPD: add powerhal reinit by sifengtian 20230711 start
typedef int (*libpowerhal_reinit)(int);
//SPD: add powerhal reinit by sifengtian 20230711 end

/* function pointer to perfserv client */
static int (*libpowerhal_NotifyAppState)(const char*, const char*, int, int, int) = NULL;
static int (*libpowerhal_UserScnDisableAll)(void) = NULL;
static int (*libpowerhal_UserScnRestoreAll)(void) = NULL;
static int (*libpowerhal_SetSysInfo)(int, const char*) = NULL;
static int (*libpowerhal_UserGetCapability)(int, int) = NULL;
static int (*libpowerhal_LockAcq)(int*, int, int, int, int, int) = NULL;
static int (*libpowerhal_CusLockHint)(int, int, int) = NULL;
static int (*libpowerhal_CusLockHint_data)(int, int, int, int*, int) = NULL;
static int (*libpowerhal_LockRel)(int) = NULL;
static int (*libpowerhal_Init)(int) = NULL;
static int (*libpowerhal_SetScnCallback)(int, const std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback> &) = NULL;
//SPD: add powerhal reinit by sifengtian 20230711 start
static int (*libpowerhal_ReInit)(int) = NULL;
//SPD: add powerhal reinit by sifengtian 20230711 end
/*****************
   Function
 *****************/
static int load_api(void)
{
    void *handle, *func;
    static int init = 0;

    if (init == 1)
        return 0;

    init = 1;

    handle = dlopen(LIB_FULL_NAME, RTLD_NOW | RTLD_GLOBAL);
    if (handle == NULL) {
        LOG_E("dlopen error: %s", dlerror());
        return -1;
    }

    func = dlsym(handle, "libpowerhal_Init");
    libpowerhal_Init = (libpowerhal_init)(func);

    if (libpowerhal_Init == NULL) {
        LOG_E("libpowerhal_Init error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_LockAcq");
    libpowerhal_LockAcq = (libpowerhal_lock_acq)(func);

    if (libpowerhal_LockAcq == NULL) {
        LOG_E("libpowerhal_LockAcq error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_CusLockHint");
    libpowerhal_CusLockHint = (libpowerhal_cus_lock_hint)(func);

    if (libpowerhal_CusLockHint == NULL) {
        LOG_E("libpowerhal_CusLockHint error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_CusLockHint_data");
    libpowerhal_CusLockHint_data = (libpowerhal_cus_lock_hint_data)(func);

    if (libpowerhal_CusLockHint_data == NULL) {
        LOG_E("libpowerhal_CusLockHint_data error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_LockRel");
    libpowerhal_LockRel = (libpowerhal_lock_rel)(func);

    if (libpowerhal_LockRel == NULL) {
        LOG_E("libpowerhal_LockRel error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_NotifyAppState");
    libpowerhal_NotifyAppState = (libpowerhal_notify_state)(func);

    if (libpowerhal_NotifyAppState == NULL) {
        LOG_E("libpowerhal_NotifyAppState error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_UserGetCapability");
    libpowerhal_UserGetCapability = (libpowerhal_user_get_capability)(func);

    if (libpowerhal_UserGetCapability == NULL) {
        LOG_E("libpowerhal_UserGetCapability error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_UserScnDisableAll");
    libpowerhal_UserScnDisableAll = (libpowerhal_user_scn_disable_all)(func);

    if (libpowerhal_UserScnDisableAll == NULL) {
        LOG_E("libpowerhal_UserScnDisableAll error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_UserScnRestoreAll");
    libpowerhal_UserScnRestoreAll = (libpowerhal_user_scn_restore_all)(func);

    if (libpowerhal_UserScnRestoreAll == NULL) {
        LOG_E("libpowerhal_UserScnRestoreAll error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "libpowerhal_SetScnCallback");
    libpowerhal_SetScnCallback = (libpowerhal_set_ScnCallback)(func);

    if (libpowerhal_SetScnCallback == NULL) {
        LOG_E("libpowerhal_SetScnCallback error: %s", dlerror());
        dlclose(handle);
        return -1;
    }
    LOG_I("libpowerhal_SetScnCallback done");

    func = dlsym(handle, "libpowerhal_SetSysInfo");
    libpowerhal_SetSysInfo = (libpowerhal_set_sysInfo)(func);

    if (libpowerhal_SetSysInfo == NULL) {
        LOG_E("libpowerhal_SetSysInfo error: %s", dlerror());
        dlclose(handle);
        return -1;
    }
    //SPD: add powerhal reinit by sifengtian 20230711 start
    func = dlsym(handle, "libpowerhal_ReInit");
    libpowerhal_ReInit = (libpowerhal_reinit)(func);

    if (libpowerhal_ReInit == NULL) {
        LOG_E("libpowerhal_ReInit error: %s", dlerror());
        dlclose(handle);
        return -1;
    }
    //SPD: add powerhal reinit by sifengtian 20230711 start
    return 0;
}

void libpowerhal_wrap_Init(int needWorker)
{
    LOG_V("");
    load_api();
    if (libpowerhal_Init) {
        libpowerhal_Init(needWorker);
    }
}

int libpowerhal_wrap_UserScnDisableAll(void)
{
    LOG_V("");
    if (libpowerhal_UserScnDisableAll) {
        return libpowerhal_UserScnDisableAll();
    }
    return 0;
}

int libpowerhal_wrap_UserScnRestoreAll(void)
{
    LOG_V("");
    if (libpowerhal_UserScnRestoreAll) {
        return libpowerhal_UserScnRestoreAll();
    }
    return 0;
}

int libpowerhal_wrap_LockAcq(int *list, int handle, int size, int pid, int tid, int duration)
{
    LOG_V("");
    if (libpowerhal_LockAcq) {
        return libpowerhal_LockAcq(list, handle, size, pid, tid, duration);
    }
    return 0;
}

int libpowerhal_wrap_CusLockHint(int hint, int duration, int pid)
{
    LOG_V("");
    if (libpowerhal_CusLockHint) {
        return libpowerhal_CusLockHint(hint, duration, pid);
    }
    return 0;
}

int libpowerhal_wrap_CusLockHint_data(int hint, int duration, int pid, int *extendedList, int extendedListSize)
{
    LOG_V("");
    if (libpowerhal_CusLockHint_data) {
        return libpowerhal_CusLockHint_data(hint, duration, pid, extendedList, extendedListSize);
    }
    return 0;
}

int libpowerhal_wrap_LockRel(int handle)
{
    LOG_V("");
    if (libpowerhal_LockRel) {
        return libpowerhal_LockRel(handle);
    }
    return 0;
}

int libpowerhal_wrap_NotifyAppState(const char *packName, const char *actName, int state, int pid, int uid)
{
    LOG_V("");
    if (libpowerhal_NotifyAppState)
        return libpowerhal_NotifyAppState(packName, actName, state, pid, uid);
    return 0;
}

int libpowerhal_wrap_SetSysInfo(int type, const char *data)
{
    LOG_V("");
    if (libpowerhal_SetSysInfo)
        return libpowerhal_SetSysInfo(type, data);
    return 0;
}

int libpowerhal_wrap_UserGetCapability(int cmd, int id)
{
    LOG_V("");
    if (libpowerhal_UserGetCapability)
        return libpowerhal_UserGetCapability(cmd, id);
    return 0;
}

int libpowerhal_wrap_SetScnUpdateCallback(int scn, const std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback>& callback)
{

    LOG_E("set scn callback  -1   %x",scn);
    if (libpowerhal_SetScnCallback){
        LOG_I("set scn callback - done");
        return libpowerhal_SetScnCallback(scn, callback);
    }
    return 0;
}

//SPD: add powerhal reinit by sifengtian 20230711 start
int libpowerhal_wrap_ReInit(int vNum)
{
    LOG_V("");
    ALOGI("tsf:start libpowerhal_ReInit");
    load_api();
    if (libpowerhal_ReInit)
        return libpowerhal_ReInit(vNum);
    return 0;
}
//SPD: add powerhal reinit by sifengtian 20230711 end

//#ifdef __cplusplus
//}
//#endif

