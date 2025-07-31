
#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>


/*** PROJECT INCLUDES ********************************************************/
#include "mi_types.h"
#include "mi_util.h"
#include "ports.h"
#include "ptimer.h"

#include "power_util.h"
#include "powerd_core.h"
#include "power_ipc.h"
#include "powerd_cmd.h"


/*** MACROS ******************************************************************/
#define PS_IPC_COM_EVENTBUF_LEN   1024

#define PS_IPC_COM_CONTENT_MAX_LEN 65535

#define PS_SCN_CFG_TYPE_LEN 2
#define PS_SCN_CFG_LEN_LEN  2

#define INVALID_HANDLE (-123456)

/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/
typedef int (*tConfigHandler) (const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen);

typedef struct tConfigHanderMap
{
   int Idx;
   tConfigHandler pHandler;
} tConfigHanderMap;

static int _PS_Cfg_Hdler_MEMORY(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen);
static int _PS_Cfg_Hdler_PERF_LOCK(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen);
static int _PS_Cfg_Hdler_LOCK_REL(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen);
static int _PS_Cfg_Hdler_PERF_CUS_LOCK_HINT(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen);
static int _PS_Cfg_Hdler_TIMER_REQ(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen);

typedef int (*tConfigHandler_marshallbuf_len) (const tPS_CMD * pCmd, int * pBufferLen);
typedef int (*tConfigHandler_marshall) (const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen);

typedef struct tConfigHanderMarshallMap
{
   int Idx;
   tConfigHandler_marshallbuf_len pBufLenHandler;
   tConfigHandler_marshall pMarshallHandler;
} tConfigHanderMarshallMap;

static int _PS_Cfg_Hdler_MarBufLen_MEMORY(const tPS_CMD * pCmd, int * pBufferLen);
static int _PS_Cfg_Hdler_MarBufLen_PERF_LOCK(const tPS_CMD * pCmd, int * pBufferLen);
static int _PS_Cfg_Hdler_MarBufLen_LOCK_REL(const tPS_CMD * pCmd, int * pBufferLen);
static int _PS_Cfg_Hdler_MarBufLen_PERF_CUS_LOCK_HINT(const tPS_CMD * pCmd, int * pBufferLen);
static int _PS_Cfg_Hdler_MarBufLen_TIMER_REQ(const tPS_CMD * pCmd, int * pBufferLen);

static int _PS_Cfg_Hdler_Mar_MEMORY(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen);
static int _PS_Cfg_Hdler_Mar_PERF_LOCK(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen);
static int _PS_Cfg_Hdler_Mar_LOCK_REL(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen);
static int _PS_Cfg_Hdler_Mar_PERF_CUS_LOCK_HINT(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen);
static int _PS_Cfg_Hdler_Mar_TIMER_REQ(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen);


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/
static tConfigHanderMap _ConfigHandlerMapper[] =
{
   {PS_SCN_TYPE_MEMORY, _PS_Cfg_Hdler_MEMORY},
   {PS_SCN_TYPE_PERF_LOCK, _PS_Cfg_Hdler_PERF_LOCK},
   {PS_SCN_TYPE_LOCK_REL, _PS_Cfg_Hdler_LOCK_REL},
   {PS_SCN_TYPE_PERF_CUS_LOCK_HINT, _PS_Cfg_Hdler_PERF_CUS_LOCK_HINT},
   {PS_SCN_TYPE_TIMER_REQ, _PS_Cfg_Hdler_TIMER_REQ},
};

static const int _CntofConfigHandlerMapper = sizeof(_ConfigHandlerMapper)/sizeof(struct tConfigHanderMap);

static tConfigHanderMarshallMap _ConfigHandlerMarshallMapper[] =
{
   {PS_SCN_TYPE_MEMORY, _PS_Cfg_Hdler_MarBufLen_MEMORY, _PS_Cfg_Hdler_Mar_MEMORY},
   {PS_SCN_TYPE_PERF_LOCK, _PS_Cfg_Hdler_MarBufLen_PERF_LOCK, _PS_Cfg_Hdler_Mar_PERF_LOCK},
   {PS_SCN_TYPE_LOCK_REL, _PS_Cfg_Hdler_MarBufLen_LOCK_REL, _PS_Cfg_Hdler_Mar_LOCK_REL},
   {PS_SCN_TYPE_PERF_CUS_LOCK_HINT, _PS_Cfg_Hdler_MarBufLen_PERF_CUS_LOCK_HINT, _PS_Cfg_Hdler_Mar_PERF_CUS_LOCK_HINT},
   {PS_SCN_TYPE_TIMER_REQ, _PS_Cfg_Hdler_MarBufLen_TIMER_REQ, _PS_Cfg_Hdler_Mar_TIMER_REQ},
};

static const int _CntofConfigHandlerMarshallMapper = sizeof(_ConfigHandlerMarshallMapper)/sizeof(struct tConfigHanderMarshallMap);


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
static int _PS_Cfg_Hdler_MarBufLen_MEMORY(const tPS_CMD * pCmd, int * pBufferLen)
{
   *pBufferLen = 0;

   if (pCmd->pMSG != NULL)
   {
      *pBufferLen = 2 + 2 + sizeof(void*);
   }

   return 0;
}

static int _PS_Cfg_Hdler_MarBufLen_PERF_LOCK(const tPS_CMD * pCmd, int * pBufferLen)
{
   *pBufferLen = 0;

   if (pCmd->perfLock.hdl != INVALID_HANDLE)
   {
      *pBufferLen = 2 + 2 + sizeof(struct tPerfLockData);
   }

   return 0;
}

static int _PS_Cfg_Hdler_MarBufLen_LOCK_REL(const tPS_CMD * pCmd, int * pBufferLen)
{
   *pBufferLen = 0;

   if (pCmd->hdlData.hdl != INVALID_HANDLE)
   {
      *pBufferLen = 2 + 2 + sizeof(struct tLockHdlData);
   }

   return 0;
}

static int _PS_Cfg_Hdler_MarBufLen_PERF_CUS_LOCK_HINT(const tPS_CMD * pCmd, int * pBufferLen)
{
   *pBufferLen = 0;

   if (pCmd->cusLockHint.hint != INVALID_HANDLE)
   {
      *pBufferLen = 2 + 2 + sizeof(struct tCusLockData);
   }

   return 0;
}

static int _PS_Cfg_Hdler_MarBufLen_TIMER_REQ(const tPS_CMD * pCmd, int * pBufferLen)
{
   *pBufferLen = 0;

   if (pCmd->timerData.type != INVALID_HANDLE)
   {
      *pBufferLen = 2 + 2 + sizeof(struct tTimerData);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int _PS_Cfg_Hdler_Mar_MEMORY(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen)
{
   unsigned short vU16;

   *pUsedLen = 0;

   if (pCmd->pMSG == NULL || BufferLen < 8)
   {
      return 0;
   }

   vU16 = htons(PS_SCN_TYPE_MEMORY);
   memcpy(pBuffer, (char *) (&vU16), 2);

   vU16 = htons(sizeof(void*));
   memcpy(pBuffer + 2, &vU16, 2);

   memcpy(pBuffer + 4, &(pCmd->pMSG), sizeof(void*));

   *pUsedLen = 2 + 2 + sizeof(void*);

   return 0;
}

static int _PS_Cfg_Hdler_Mar_PERF_LOCK(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen)
{
   unsigned short vU16;
   unsigned long vU32;
   int i;

   *pUsedLen = 0;

   if (pCmd->perfLock.hdl == INVALID_HANDLE || BufferLen < 8)
   {
      return 0;
   }

   /*
   struct tPerfLockData {
       int hdl;
       int pid;
       int uid;
       int duration;
       int reserved;
       int size;
       int rscList[MAX_ARGS_PER_REQUEST];

   };
   */

   vU16 = htons(PS_SCN_TYPE_PERF_LOCK);
   memcpy(pBuffer, (char *) (&vU16), 2);

   vU16 = htons(sizeof(struct tPerfLockData));
   memcpy(pBuffer + 2, &vU16, 2);

   vU32 = htonl(pCmd->perfLock.hdl);
   memcpy(pBuffer + 4, &vU32, 4);
   vU32 = htonl(pCmd->perfLock.pid);
   memcpy(pBuffer + 8, &vU32, 4);
   vU32 = htonl(pCmd->perfLock.uid);
   memcpy(pBuffer + 12, &vU32, 4);
   vU32 = htonl(pCmd->perfLock.duration);
   memcpy(pBuffer + 16, &vU32, 4);
   vU32 = htonl(pCmd->perfLock.reserved);
   memcpy(pBuffer + 20, &vU32, 4);
   vU32 = htonl(pCmd->perfLock.size);
   memcpy(pBuffer + 24, &vU32, 4);

   for (i=0; i < MAX_ARGS_PER_REQUEST && i < pCmd->perfLock.size; i++) {
       vU32 = htonl(pCmd->perfLock.rscList[i]);
       memcpy(pBuffer + 28 + i * 4, &vU32, 4);
   }

   *pUsedLen = 2 + 2 + sizeof(struct tPerfLockData);

   return 0;
}

static int _PS_Cfg_Hdler_Mar_LOCK_REL(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen)
{
   unsigned short vU16;
   unsigned long vU32;

   *pUsedLen = 0;

   if (pCmd->hdlData.hdl == INVALID_HANDLE || BufferLen < 8)
   {
      return 0;
   }

   /*
    struct tLockHdlData {
        int hdl;
        int reserved;
    };
   */

   vU16 = htons(PS_SCN_TYPE_LOCK_REL);
   memcpy(pBuffer, (char *) (&vU16), 2);

   vU16 = htons(sizeof(struct tLockHdlData));
   memcpy(pBuffer + 2, &vU16, 2);

   vU32 = htonl(pCmd->hdlData.hdl);
   memcpy(pBuffer + 4, &vU32, 4);
   vU32 = htonl(pCmd->hdlData.reserved);
   memcpy(pBuffer + 8, &vU32, 4);

   *pUsedLen = 2 + 2 + sizeof(struct tLockHdlData);

   return 0;
}

static int _PS_Cfg_Hdler_Mar_PERF_CUS_LOCK_HINT(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen)
{
   unsigned short vU16;
   unsigned long vU32;

   *pUsedLen = 0;

   if (pCmd->cusLockHint.hint == INVALID_HANDLE || BufferLen < 8)
   {
      return 0;
   }

   vU16 = htons(PS_SCN_TYPE_PERF_CUS_LOCK_HINT);
   memcpy(pBuffer, (char *) (&vU16), 2);

   vU16 = htons(sizeof(struct tCusLockData));
   memcpy(pBuffer + 2, &vU16, 2);

   /*
   int hdl;
   int pid;
   int hint;
   int duration;
    */
   vU32 = htonl(pCmd->cusLockHint.hdl);
   memcpy(pBuffer + 4, &vU32, 4);
   vU32 = htonl(pCmd->cusLockHint.pid);
   memcpy(pBuffer + 8, &vU32, 4);
   vU32 = htonl(pCmd->cusLockHint.hint);
   memcpy(pBuffer + 12, &vU32, 4);
   vU32 = htonl(pCmd->cusLockHint.duration);
   memcpy(pBuffer + 16, &vU32, 4);

   *pUsedLen = 2 + 2 + sizeof(struct tCusLockData);

   return 0;
}

static int _PS_Cfg_Hdler_Mar_TIMER_REQ(const tPS_CMD * pCmd, char * pBuffer, int BufferLen, int * pUsedLen)
{
   unsigned short vU16;
   unsigned long vU32;

   *pUsedLen = 0;

   if (pCmd->timerData.type == INVALID_HANDLE || BufferLen < 8)
   {
      return 0;
   }

   vU16 = htons(PS_SCN_TYPE_TIMER_REQ);
   memcpy(pBuffer, (char *) (&vU16), 2);

   vU16 = htons(sizeof(struct tTimerData));
   memcpy(pBuffer + 2, &vU16, 2);

   /*
   int type;
   int data;
   int duration;
    */
   vU32 = htonl(pCmd->timerData.type);
   memcpy(pBuffer + 4, &vU32, 4);
   vU32 = htonl(pCmd->timerData.data);
   memcpy(pBuffer + 8, &vU32, 4);
   vU32 = htonl(pCmd->timerData.duration);
   memcpy(pBuffer + 12, &vU32, 4);

   *pUsedLen = 2 + 2 + sizeof(struct tTimerData);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
static int _PS_Cfg_Hdler_MEMORY(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen)
{
   unsigned short vLen;

   memcpy(&vLen, pData + PS_SCN_CFG_TYPE_LEN, PS_SCN_CFG_LEN_LEN);

   vLen = ntohs(vLen);

   if (vLen != sizeof(void*) || Datalen < vLen + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN)
   {
         TWPCDBGP("_PS_Cfg_Hdler_MEMORY\n");
      // to do, error check
   }
   else
   {
      memcpy(&pCmd->pMSG, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN, vLen);
   }

   *pUsedLen = PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + vLen;

   return 0;
}

static int _PS_Cfg_Hdler_PERF_LOCK(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen)
{
   unsigned short vLen;
   unsigned long vLong;
   int i;

   memcpy(&vLen, pData + PS_SCN_CFG_TYPE_LEN, PS_SCN_CFG_LEN_LEN);

   vLen = ntohs(vLen);

   if (vLen != sizeof(struct tPerfLockData) || Datalen < vLen + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN)
   {
         TWPCDBGP("_PS_Cfg_Hdler_PERF_LOCK\n");
      // to do, error check
   }
   else
   {
     /*
        struct tPerfLockData {
            int hdl;
            int pid;
            int uid;
            int duration;
            int reserved;
            int size;
            int rscList[MAX_ARGS_PER_REQUEST];
        };
     */
      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN, 4);
      pCmd->perfLock.hdl = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 4, 4);
      pCmd->perfLock.pid = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 8, 4);
      pCmd->perfLock.uid = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 12, 4);
      pCmd->perfLock.duration = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 16, 4);
      pCmd->perfLock.reserved = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 20, 4);
      pCmd->perfLock.size = ntohl(vLong);

      for (i=0; i < MAX_ARGS_PER_REQUEST && i < pCmd->perfLock.size; i++) {
          memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 24 + i * 4, 4);
          pCmd->perfLock.rscList[i] = ntohl(vLong);
      }
   }

   *pUsedLen = PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + vLen;

   return 0;
}

static int _PS_Cfg_Hdler_LOCK_REL(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen)
{
   unsigned short vLen;
   unsigned long vLong;

   memcpy(&vLen, pData + PS_SCN_CFG_TYPE_LEN, PS_SCN_CFG_LEN_LEN);

   vLen = ntohs(vLen);

   if (vLen != sizeof(struct tLockHdlData) || Datalen < vLen + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN)
   {
         TWPCDBGP("_PS_Cfg_Hdler_LOCK_REL\n");
      // to do, error check
   }
   else
   {
     /*
        struct tLockHdlData {
            int hdl;
            int reserved;
        };
     */
      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN, 4);
      pCmd->hdlData.hdl = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 4, 4);
      pCmd->hdlData.reserved = ntohl(vLong);
   }

   *pUsedLen = PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + vLen;

   return 0;
}

static int _PS_Cfg_Hdler_PERF_CUS_LOCK_HINT(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen)
{
   unsigned short vLen;
   unsigned long vLong;

   memcpy(&vLen, pData + PS_SCN_CFG_TYPE_LEN, PS_SCN_CFG_LEN_LEN);

   vLen = ntohs(vLen);

   if (vLen != sizeof(struct tCusLockData) || Datalen < vLen + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN)
   {
         TWPCDBGP("_PS_Cfg_Hdler_PERF_CUS_LOCK_HINT\n");
      // to do, error check
   }
   else
   {
     /*
        int hdl;
        int pid;
        int hint;
        int duration;
     */
      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN, 4);
      pCmd->cusLockHint.hdl = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 4, 4);
      pCmd->cusLockHint.pid = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 8, 4);
      pCmd->cusLockHint.hint = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 12, 4);
      pCmd->cusLockHint.duration = ntohl(vLong);
   }

   *pUsedLen = PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + vLen;

   return 0;
}

static int _PS_Cfg_Hdler_TIMER_REQ(const char * pData, int Datalen, tPS_CMD * pCmd, int * pUsedLen)
{
   unsigned short vLen;
   unsigned long vLong;

   memcpy(&vLen, pData + PS_SCN_CFG_TYPE_LEN, PS_SCN_CFG_LEN_LEN);

   vLen = ntohs(vLen);

   if (vLen != sizeof(struct tTimerData) || Datalen < vLen + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN)
   {
         TWPCDBGP("_PS_Cfg_Hdler_TIMER_REQ\n");
      // to do, error check
   }
   else
   {
     /*
        int type;
        int data;
        int duration;
     */
      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN, 4);
      pCmd->timerData.type = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 4, 4);
      pCmd->timerData.data = ntohl(vLong);

      memcpy(&vLong, pData + PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + 8, 4);
      pCmd->timerData.duration = ntohl(vLong);
   }

   *pUsedLen = PS_SCN_CFG_TYPE_LEN + PS_SCN_CFG_LEN_LEN + vLen;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
static int _power_ipc_type_handler_msg(tPS_IPC_RecvJob * pJob, tPS_CMD ** ppCmd)
{
   tPS_CMD * vpCmd = NULL;
   int vI;
   unsigned long vHandledLen = 0;
   char * vpCfg;
   unsigned short vCfgType;

   int vUsedLen;
   int vRet;

   vRet = powerd_cmd_create((void **) &vpCmd);
   if (vRet || vpCmd == NULL)
      return -1;

   vpCmd->CmdID = pJob->CAMCOMType;

   vpCfg = pJob->pContent;

   while (vHandledLen < pJob->ContentLen)
   {
      vUsedLen = 0;

      if (pJob->ContentLen >= PS_SCN_CFG_TYPE_LEN) {
         memcpy(&vCfgType, vpCfg, PS_SCN_CFG_TYPE_LEN);
      } else {
         free(vpCmd);
         return -1;
      }

      vCfgType = ntohs(vCfgType);

      //TWPCDBGP("%d\n", vCfgType);

      for (vI = 0; vI < _CntofConfigHandlerMapper; vI++)
      {
         if (_ConfigHandlerMapper[vI].Idx == vCfgType)
         {
            vRet = _ConfigHandlerMapper[vI].pHandler(vpCfg, pJob->ContentLen - vHandledLen, vpCmd, &vUsedLen);

            if (vRet)
            {
               // to do, error check
               TWPCDBGP("_power_ipc_type_handler_msg\n");
            }
            else
            {
               vHandledLen += vUsedLen;
               vpCfg = vpCfg + vUsedLen;
            }

            break;
         }
      }

      if (vI == _CntofConfigHandlerMapper)
      {
         TWPCDBGP("vCfgType:%d not found\n", vCfgType);
         break;
      }
   }

   *ppCmd = vpCmd;

   return 0;
}


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
int powerd_ipc_init_pscmd(tPS_CMD * pCmd)
{
   pCmd->pMSG = NULL;
   pCmd->cusLockHint.hint = INVALID_HANDLE;
   pCmd->perfLock.hdl = INVALID_HANDLE;
   pCmd->hdlData.hdl = INVALID_HANDLE;
   pCmd->timerData.type = INVALID_HANDLE;

   return 0;
}

int powerd_ipc_recvcmd(tPS_IPC_RecvJob * pJob, int PSDc_fd)
{
   unsigned long vWantRecvLen;
   int vRet = 0;

   if (pJob == NULL)
   {
      return -1;
   }

   if (pJob->State == PS_IPC_COM_READY)
   {
      return 0;
   }

SecondRound:

   if (pJob->RecvLen < PS_IPC_COM_HEADER_LEN)
   {
      vWantRecvLen = PS_IPC_COM_HEADER_LEN - pJob->RecvLen;

      vRet = recv(PSDc_fd, pJob->Header + pJob->RecvLen, vWantRecvLen, 0);
   }
   else if (pJob->ContentLen > 0)
   {
      vWantRecvLen = pJob->ContentLen + PS_IPC_COM_HEADER_LEN - pJob->RecvLen;

      vRet = recv(PSDc_fd, pJob->pContent + (pJob->RecvLen - PS_IPC_COM_HEADER_LEN), vWantRecvLen, 0);
   }
   else
   {
      // should not happen
      TWPCDBGP("powerd_ipc_recvcmd should not happen\n");
      return -1;
   }

   //TWPCDBGP("vWantRecvLen %lu, vRet %d\n", vWantRecvLen, vRet);

   if (vRet <= 0)
   {
      TWPCDBGP("powerd_ipc_recvcmd vRet %d errno %d, %s\n", vRet, errno, strerror(errno));

      #if 0
      if (vRet == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
      {
         return 0;
      }
      #endif

      return -1;
   }

   if (pJob->RecvLen < PS_IPC_COM_HEADER_LEN)
   {
      unsigned long vLenParam;

      pJob->RecvLen += vRet;

      if (pJob->RecvLen < PS_IPC_COM_HEADER_LEN)
      {
         TWPCDBGP("powerd_ipc_recvcmd pJob->RecvLen < PS_IPC_COM_HEADER_LEN\n");
         return -1;
      }

      memcpy(&vLenParam, pJob->Header + 4, 4);

      vLenParam = ntohl(vLenParam);

      if (vLenParam <= PS_IPC_COM_CONTENT_MAX_LEN)
      {
         if (vLenParam > 0)
         {
            pJob->pContent = (char *) malloc(vLenParam);
            if (pJob->pContent == NULL) {
                TWPCDBGP("powerd_ipc_recvcmd pJob->pContent == NULL\n");
                return -1;
            }
            pJob->ContentLen = vLenParam;

            goto SecondRound;
         }
      }
      else
      {
         TWPCDBGP("powerd_ipc_recvcmd vLenParam > PS_IPC_COM_CONTENT_MAX_LEN\n");
         return -1;
      }
   }
   else if (pJob->ContentLen > 0)
   {
      pJob->RecvLen += vRet;

      vWantRecvLen = pJob->ContentLen + PS_IPC_COM_HEADER_LEN - pJob->RecvLen;

      if (vWantRecvLen > 0)
      {
         TWPCDBGP("powerd_ipc_recvcmd unexpected vWantRecvLen:%lu\n", vWantRecvLen);
         return -1;
      }
   }

   memcpy(&pJob->CAMCOMType, pJob->Header + 2, 2);

   pJob->CAMCOMType = ntohs(pJob->CAMCOMType);
   pJob->State = PS_IPC_COM_READY;

   return 0;
}

int powerd_ipc_sendcmd(tPS_IPC_SendJob * pJob, int PSDc_fd)
{
    if(send(PSDc_fd, pJob->pData, pJob->DataLen, MSG_NOSIGNAL) < 0)
        return -1;

    return 0;
}

int powerd_cmd_unmarshall(tPS_IPC_RecvJob * pJob, tPS_CMD ** ppCmd)
{
   *ppCmd = NULL;

   // to do, check job's state

   switch (pJob->CAMCOMType)
   {
      case PS_IPC_COM_TYPE_MSG:
      case PS_IPC_COM_TYPE_PERF_LOCK_ACQ:
      case PS_IPC_COM_TYPE_PERF_LOCK_REL:
      case PS_IPC_COM_TYPE_PERF_CUS_LOCK_HINT:
      case PS_IPC_COM_TYPE_PERF_TIMER_REQ:
         _power_ipc_type_handler_msg(pJob, ppCmd);
         break;

      default:
         // to do, may be registerd by others, for now just error
         TWPCDBGP("Unhandled Command Type %d at cmd_unmarshall\n", pJob->CAMCOMType);
         return -1;
         break;
   }

   return 0;
}

int powerd_cmd_marshall(tPS_IPC_SendJob * pJob, tPS_CMD * pCmd)
{
   int vI;
   int vRet;
   int vBufLen = 8;
   char * vpBuf = NULL;

   char * vpCfgBuf = NULL;
   int vCfgBufLen;
   int vCfgUsedLen;

   unsigned short vU16;
   unsigned long vU32;

   if (pJob == NULL || pCmd == NULL)
      return -1;

   // 1. Predict buffer len
   for (vI = 0; vI < _CntofConfigHandlerMarshallMapper; vI++)
   {
      vRet = _ConfigHandlerMarshallMapper[vI].pBufLenHandler(pCmd, &vCfgUsedLen);

      vBufLen += vCfgUsedLen;
   }

   if(vBufLen > 0)
      vpBuf = (char *) malloc(vBufLen);

   if (vpBuf == NULL)
      return -1;

   // keep header
   vpCfgBuf = vpBuf + 8;
   vCfgBufLen = vBufLen - 8;

   // marshall to buffer
   // 1. Predict buffer len
   for (vI = 0; vI < _CntofConfigHandlerMarshallMapper; vI++)
   {
      vRet = _ConfigHandlerMarshallMapper[vI].pMarshallHandler(pCmd, vpCfgBuf, vCfgBufLen, &vCfgUsedLen);

      vpCfgBuf += vCfgUsedLen;
      vCfgBufLen -= vCfgUsedLen;
   }

   // write header
   memset(vpBuf, 0, 2);

   vU16 = htons(pCmd->CmdID);
   memcpy(vpBuf + 2, &vU16, 2);

   vU32 = htonl(vBufLen - 8);
   memcpy(vpBuf + 4, &vU32, 4);

   // fill SendJob
   pJob->pData = vpBuf;
   pJob->DataLen = vBufLen;

   return 0;
}

int powerd_ipc_resetRecvJob(tPS_IPC_RecvJob * pJob)
{
   if (pJob->pContent)
      free(pJob->pContent);

   pJob->pContent = NULL;
   pJob->ContentLen = 0;

   return 0;
}

int powerd_ipc_resetSendJob(tPS_IPC_SendJob * pJob)
{
   if (pJob->pData)
      free(pJob->pData);

   pJob->pData = NULL;
   pJob->DataLen = 0;

   return 0;
}


#ifdef __cplusplus
}
#endif

