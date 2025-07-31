
#ifdef __cplusplus
extern "C" {
#endif


/*** STANDARD INCLUDES *******************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <sys/un.h>


/*** PROJECT INCLUDES ********************************************************/
#include "ports.h"
#include "mi_types.h"
#include "mi_util.h"

#include "power_util.h"
#include "powerd_cmd.h"
#include "powerd_core.h"
#include "power_ipc.h"


/*** MACROS ******************************************************************/
#define _TCP_BIND_ADDR_ "com.mediatek.powerhald"


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
static int _connect_powerd(void)
{
   int vRet;
   vRet = socket_local_client(_TCP_BIND_ADDR_, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

   if (vRet < 0)
   {
      TWPCDBGP("Connect to %s failed.\n", _TCP_BIND_ADDR_);
   }

   return vRet;
}

static int _waitRsploop(tPS_IPC_RecvJob * pRecvJob, int CliFD, int Timeout)
{
   int vRet;

   pRecvJob->State = PS_IPC_COM_NULL;

   while (pRecvJob->State == PS_IPC_COM_NULL)
   {
      vRet = powerd_ipc_recvcmd(pRecvJob, CliFD);

      if (vRet == -1)
      {
         // socket error, release
         TWPCDBGP("timeout:%d\n", Timeout);

         return -1;
      }
   }

   return 0;
}


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
int power_cmd_create(void ** ppCmd)
{
   if (!ppCmd)
      return 1;

   return powerd_cmd_create(ppCmd);
}

int power_cmd_destory(void * pCmd)
{
   if (!pCmd)
      return 1;

   return powerd_cmd_destory(pCmd);
}

long power_msg(void * pMsg, void **ppRspMsg)
{
   long vRet = 0;
   int vFD;
   tPS_IPC_SendJob vSendJob;
   tPS_IPC_RecvJob vRecvJob;

   struct tPS_CMD * vpCmd = NULL, * vpRspCmd = NULL;

   power_cmd_create((void **) &vpCmd);
   if(vpCmd == NULL)
     return -1;

   vpCmd->CmdID = PS_IPC_COM_TYPE_MSG;
   vpCmd->pMSG = pMsg;

   memset(&vSendJob, 0, sizeof(tPS_IPC_SendJob));
   memset(&vRecvJob, 0, sizeof(tPS_IPC_RecvJob));

   powerd_cmd_marshall(&vSendJob, vpCmd);

   vFD = _connect_powerd();
   if (vFD < 0)
   {
      vRet = -1;
      goto exit;
   }

   powerd_ipc_sendcmd(&vSendJob, vFD);

   // wait rsp
   _waitRsploop(&vRecvJob, vFD, 10);

   // unmarshall to cmd
   vRet = powerd_cmd_unmarshall(&vRecvJob, &vpRspCmd);

   if (vRet)
   {
      vRet = -1;
      goto exit;
   }

//   vRet = vpRspCmd->Handle;

   *ppRspMsg = vpRspCmd->pMSG;

   powerd_cmd_destory(vpRspCmd);

exit:
   if (vFD >= 0)
   {
      close(vFD);
   }

   powerd_cmd_destory(vpCmd);

   powerd_ipc_resetSendJob(&vSendJob);
   powerd_ipc_resetRecvJob(&vRecvJob);

   return vRet;
}

long power_msg_perf_lock_acq(void * pMsg, void *pRspMsg)
{
   long vRet = 0;
   int vFD;
   tPS_IPC_SendJob vSendJob;
   tPS_IPC_RecvJob vRecvJob;

   struct tPS_CMD * vpCmd, * vpRspCmd;

   power_cmd_create((void **) &vpCmd);
   if(vpCmd == NULL)
     return -1;

   vpCmd->CmdID = PS_IPC_COM_TYPE_PERF_LOCK_ACQ;
   vpCmd->pMSG = NULL;
   memcpy(&(vpCmd->perfLock), ((struct tPowerData*)pMsg)->pBuf, sizeof(struct tPerfLockData));

   //TWPCDBGP("%s hdl:%d, duration:%d, size:%d", __FUNCTION__, vpCmd->perfLock.hdl, vpCmd->perfLock.duration, vpCmd->perfLock.size);

   memset(&vSendJob, 0, sizeof(tPS_IPC_SendJob));
   memset(&vRecvJob, 0, sizeof(tPS_IPC_RecvJob));

   powerd_cmd_marshall(&vSendJob, vpCmd);

   vFD = _connect_powerd();
   if (vFD < 0)
   {
      vRet = -1;
      goto exit;
   }

   powerd_ipc_sendcmd(&vSendJob, vFD);

   // wait rsp
   _waitRsploop(&vRecvJob, vFD, 10);

   // unmarshall to cmd
   vRet = powerd_cmd_unmarshall(&vRecvJob, &vpRspCmd);

   if (vRet)
   {
      vRet = -1;
      goto exit;
   }

   memcpy(((struct tPowerData*)pRspMsg)->pBuf, &(vpRspCmd->perfLock), sizeof(struct tPerfLockData));

   powerd_cmd_destory(vpRspCmd);

exit:
   if (vFD >= 0)
   {
      close(vFD);
   }

   powerd_cmd_destory(vpCmd);

   powerd_ipc_resetSendJob(&vSendJob);
   powerd_ipc_resetRecvJob(&vRecvJob);

   return vRet;
}

long power_msg_perf_lock_rel(void * pMsg, void *pRspMsg)
{
   long vRet = 0;
   int vFD;
   tPS_IPC_SendJob vSendJob;
   tPS_IPC_RecvJob vRecvJob;

   struct tPS_CMD * vpCmd, * vpRspCmd;

   power_cmd_create((void **) &vpCmd);
   if(vpCmd == NULL)
     return -1;

   vpCmd->CmdID = PS_IPC_COM_TYPE_PERF_LOCK_REL;
   vpCmd->pMSG = NULL;
   memcpy(&(vpCmd->hdlData), ((struct tPowerData*)pMsg)->pBuf, sizeof(struct tLockHdlData));

   //TWPCDBGP("%s hdl:%d", __FUNCTION__, vpCmd->hdlData.hdl);

   memset(&vSendJob, 0, sizeof(tPS_IPC_SendJob));
   memset(&vRecvJob, 0, sizeof(tPS_IPC_RecvJob));

   powerd_cmd_marshall(&vSendJob, vpCmd);

   vFD = _connect_powerd();
   if (vFD < 0)
   {
      vRet = -1;
      goto exit;
   }

   powerd_ipc_sendcmd(&vSendJob, vFD);

   // wait rsp
   _waitRsploop(&vRecvJob, vFD, 10);

   // unmarshall to cmd
   vRet = powerd_cmd_unmarshall(&vRecvJob, &vpRspCmd);

   if (vRet)
   {
      vRet = -1;
      goto exit;
   }

   memcpy(((struct tPowerData*)pRspMsg)->pBuf, &(vpRspCmd->hdlData), sizeof(struct tLockHdlData));

   powerd_cmd_destory(vpRspCmd);

exit:
   if (vFD >= 0)
   {
      close(vFD);
   }

   powerd_cmd_destory(vpCmd);

   powerd_ipc_resetSendJob(&vSendJob);
   powerd_ipc_resetRecvJob(&vRecvJob);

   return vRet;
}


long power_msg_cus_lock_hint(void * pMsg, void *pRspMsg)
{
   long vRet = 0;
   int vFD;
   tPS_IPC_SendJob vSendJob;
   tPS_IPC_RecvJob vRecvJob;

   struct tPS_CMD * vpCmd, * vpRspCmd;

   power_cmd_create((void **) &vpCmd);
   if(vpCmd == NULL)
     return -1;

   vpCmd->CmdID = PS_IPC_COM_TYPE_PERF_CUS_LOCK_HINT;
   vpCmd->pMSG = NULL;
   memcpy(&(vpCmd->cusLockHint), ((struct tPowerData*)pMsg)->pBuf, sizeof(struct tCusLockData));

   //TWPCDBGP("%s hint:%d, duration:%d", __FUNCTION__, vpCmd->cusLockHint.hint, vpCmd->cusLockHint.duration);

   memset(&vSendJob, 0, sizeof(tPS_IPC_SendJob));
   memset(&vRecvJob, 0, sizeof(tPS_IPC_RecvJob));

   powerd_cmd_marshall(&vSendJob, vpCmd);

   vFD = _connect_powerd();
   if (vFD < 0)
   {
      vRet = -1;
      goto exit;
   }

   powerd_ipc_sendcmd(&vSendJob, vFD);

   // wait rsp
   _waitRsploop(&vRecvJob, vFD, 10);

   // unmarshall to cmd
   vRet = powerd_cmd_unmarshall(&vRecvJob, &vpRspCmd);

   if (vRet)
   {
      vRet = -1;
      goto exit;
   }

   memcpy(((struct tPowerData*)pRspMsg)->pBuf, &(vpRspCmd->cusLockHint), sizeof(struct tCusLockData));

   powerd_cmd_destory(vpRspCmd);

exit:
   if (vFD >= 0)
   {
      close(vFD);
   }

   powerd_cmd_destory(vpCmd);

   powerd_ipc_resetSendJob(&vSendJob);
   powerd_ipc_resetRecvJob(&vRecvJob);

   return vRet;
}

long power_msg_timer_req(void * pMsg, void *pRspMsg)
{
   long vRet = 0;
   int vFD;
   tPS_IPC_SendJob vSendJob;
   tPS_IPC_RecvJob vRecvJob;

   struct tPS_CMD * vpCmd, * vpRspCmd;

   power_cmd_create((void **) &vpCmd);
   if(vpCmd == NULL)
     return -1;

   vpCmd->CmdID = PS_IPC_COM_TYPE_PERF_TIMER_REQ;
   vpCmd->pMSG = NULL;
   memcpy(&(vpCmd->timerData), ((struct tPowerData*)pMsg)->pBuf, sizeof(struct tTimerData));

   TWPCDBGP("%s", __FUNCTION__);

   memset(&vSendJob, 0, sizeof(tPS_IPC_SendJob));
   memset(&vRecvJob, 0, sizeof(tPS_IPC_RecvJob));

   powerd_cmd_marshall(&vSendJob, vpCmd);

   //TWPCDBGP("%s 1", __FUNCTION__);

   vFD = _connect_powerd();
   if (vFD < 0)
   {
      vRet = -1;
      goto exit;
   }

   //TWPCDBGP("%s 2", __FUNCTION__);
   powerd_ipc_sendcmd(&vSendJob, vFD);

   // wait rsp
   //TWPCDBGP("%s 3", __FUNCTION__);
   _waitRsploop(&vRecvJob, vFD, 10);

   // unmarshall to cmd
   //TWPCDBGP("%s 4", __FUNCTION__);
   vRet = powerd_cmd_unmarshall(&vRecvJob, &vpRspCmd);

   if (vRet)
   {
      vRet = -1;
      goto exit;
   }

   memcpy(((struct tPowerData*)pRspMsg)->pBuf, &(vpRspCmd->timerData), sizeof(struct tTimerData));

   //TWPCDBGP("%s 5", __FUNCTION__);
   powerd_cmd_destory(vpRspCmd);

exit:
   if (vFD >= 0)
   {
      close(vFD);
   }

   powerd_cmd_destory(vpCmd);

   powerd_ipc_resetSendJob(&vSendJob);
   powerd_ipc_resetRecvJob(&vRecvJob);

   return vRet;
}

#ifdef __cplusplus
}
#endif

