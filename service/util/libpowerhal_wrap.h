
#ifndef __LIBPOWERHAL_WRAP_H__
#define __LIBPOWERHAL_WRAP_H__

//#ifdef __cplusplus
//extern "C" {
//#endif

//#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPowerCallback.h>
#include <aidl/vendor/mediatek/hardware/mtkpower/BnMtkPowerService.h>

using aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService;
//using ::vendor::mediatek::hardware::mtkpower::V1_2::IMtkPowerCallback;
//using ::android::sp;

/*** STANDARD INCLUDES *******************************************************/


/*** PROJECT INCLUDES ********************************************************/


/*** MACROS ******************************************************************/

/*** GLOBAL TYPES DEFINITIONS ************************************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/
extern void libpowerhal_wrap_Init(int);
extern int libpowerhal_wrap_UserScnDisableAll(void);
extern int libpowerhal_wrap_UserScnRestoreAll(void);
extern int libpowerhal_wrap_LockAcq(int *list, int handle, int size, int pid, int tid, int duration);
extern int libpowerhal_wrap_CusLockHint(int hint, int duration, int pid);
extern int libpowerhal_wrap_CusLockHint_data(int hint, int duration, int pid, int *extendedList, int extendedListSize);
extern int libpowerhal_wrap_LockRel(int handle);
extern int libpowerhal_wrap_NotifyAppState(const char *packName, const char *actName, int state, int pid, int uid);
extern int libpowerhal_wrap_SetSysInfo(int type, const char *data);
extern int libpowerhal_wrap_UserGetCapability(int cmd, int id);
extern int libpowerhal_wrap_SetScnUpdateCallback(int scn, const std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback>& callback);

//#ifdef __cplusplus
//}
//#endif

#endif /* End of #ifndef __LIBPOWERHAL_WRAP_H__ */

