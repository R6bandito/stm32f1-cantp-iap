#ifndef __CUS_CANTP_H__
#define __CUS_CANTP_H__

#ifndef __Cus_CANTP_XzzwY7a9BBCTQ7__
  #define __Cus_CANTP_XzzwY7a9BBCTQ7__
#endif // __Cus_CANTP_XzzwY7a9BBCTQ7__

#include <stdlib.h>
#include <string.h>
#include "Cus_CANTP_cfg.h"

#define USE_CANTP_UTILIS             (0)
  #if (USE_CANTP_UTILIS)
    #include "Cus_CANTP_Utils.h"
  #endif 


/*  ---------------------------------------------------  */
typedef struct Cus_CANTP_Conn Cus_CANTp_Conn_t;
  typedef unsigned char U8;
  typedef unsigned int U32;
  typedef unsigned short U16; 

  typedef signed char S8;
  typedef signed short S16;
  typedef signed short S32;

  typedef U8 (*Cus_CanTP_CanSendFunc)( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc );
  typedef U8 (*Cus_CanTP_CanRecvFunc)( Cus_CANTp_Conn_t *pConn, U32 *pcanId, U8 *pData, U8 *pDlc );
  typedef void (*Cus_CanTP_DataIndication)( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len );
  typedef void (*Cus_CanTP_ErrCallback)( Cus_CANTp_Conn_t *pConn, U8 errcode );

  #ifndef NULL
    #define NULL ((void *)0)
  #endif // NULL
/*  ---------------------------------------------------  */



/*  ---------------------------------------------------  */
  #define API_USE_LEGACY               (0)
/*  ---------------------------------------------------  */



/*  ---------------------------------------------------  */
  #define MAX_SUPPORT_CONN             (4U)
    #define CONN_INDEX_1               (0U)
    #define CONN_INDEX_2               (1U)
    #define CONN_INDEX_3               (2U)
    #define CONN_INDEX_4               (3U)


  #define NORMAL_ADDRESS_MODE          (0U)
  #define EXT_ADDRESS_MODE             (1U)
  #define MIXED_ADDRESS_MODE           (2U)

  #define TIMER_NBS                    (200UL)
  #define TIMER_NAS                    (100UL)
  #define TIMER_NAR                    (200UL)
  #define TIMER_NCR                    (100UL)

  #define CLASSIC_CAN_TXDLC            (8)
  #define CANTP_RX_SENDER_ADDR         (0)      // 接收方,SA默认为0.
  #define CANTP_AE_NONE                (0)

  #define CANTP_ADDR_MODE_COMMON       (0)      // 普通寻址.
  #define CANTP_ADDR_MODE_EXT          (1)      // 拓展寻址.
  #define CANTP_ADDR_MODE_MIX          (2)      // 混合寻址.

  #define CANTP_TA_TYPE_PHYSICAL       (0)      // 物理寻址.
  #define CANTP_TA_TYPE_FUNCTIONAL     (1)      // 功能寻址.

/*  ---------------------------------------------------  */


typedef enum 
{
  // 空闲状态.
  CONN_IDLE = 0,

  CONN_TX_SF,     // 发送单帧. 
  CONN_TX_FF,     // 发送首帧. 
  CONN_TX_CF,     // 发送连续帧. 
  CONN_TX_WAIT_FC,  // 等待流控帧.
  CONN_TX_FC,     // 发送流控.

  CONN_RX_FF,       // 接收首帧. 预备发送流控.
  CONN_RX_CF,       // 正在接收连续帧.
  CONN_RX_WAIT_CF,  // 已发送流控，等待连续帧.
  CONN_RX_SF

} Cus_CANTP_State_t;


typedef enum {
  CUS_CANTP_ERR_NBS_TIMEOUT,      // 等待流控超时 (发送方)
  CUS_CANTP_ERR_NCR_TIMEOUT,      // 等待连续帧超时 (接收方)
  CUS_CANTP_ERR_NAS_TIMEOUT,
  CUS_CANTP_ERR_FLOW_OVFLW,       // 收到 OVFLW 流控帧
  CUS_CANTP_ERR_FLOW_WAIT,        // 收到 WAIT 流控帧
  CUS_CANTP_ERR_SN_MISMATCH,      // SN 校验失败
  CUS_CANTP_ERR_TX_FAILED,        // 发送失败（重试耗尽）
  CUS_CANTP_ERR_RX_BUFFER_FULL,   // 接收缓冲区不足

} Cus_CANTP_ErrCode_t;


typedef enum 
{
  FLOW_CTS,
  FLOW_WAIT,
  FLOW_OVFLW

} Cus_CANTP_FlowState_t;


struct Cus_CANTP_Conn
{
  Cus_CANTP_State_t CurrentState;     // 当前状态.

  U32 TxBytes;              // 已经发送的数据(字节).
  const U8 *pSendData;      // 待发送的原始数据.
  U32 RxBytes;              // 已接收到的数据(字节).
  U8 SN_Code;               // SN序列码.
  U32 TotalSize;            // 该此会话 数据总长度.
  U32 Remaining;            // 剩余要发送的字节.

  U8 STmin;                 // 流控参数. STmin.
  U16 Timer_StminDelayOnly; // STmin 延迟计数器.
  U8 BS;                    // 流控参数. BS.
  U16 RemainingBS;           // 当前块内还可以连续发送的 CF 数量.

  U32 Timer_N_As;           // 发送方帧发送确认超时计时器.
  U32 Timer_N_Bs;           // 发送方等待流控帧超时计时器.
  U32 Timer_N_Ar;           // 接收方帧发送确认超时计时器.
  U32 Timer_N_Cr;           // 接收方等待连续帧超时计时器.

  U8 *pRecvBuffer;          // 接收缓冲区.
  U32 RecvBuffer_Size;      // 缓冲区大小.
  U32 RxPos;                // 已接收数据长度(RecvBuf中下一个写入位置).
  U32 TxPos;                // 发送偏移量.

  S8 ConnIndex;             // 所属资源池ID.
  U8 ChannelConfigTabID;    // 通道配置表ID.

  U8 TxMailBoxIndex;        // 通信所用的发送邮箱号.(0/1/2) FF表示初始化误状态. 
  void *BindCANDevice;      // 绑定的底层CAN设备. (请通过类型转换将其转换为 CAN_TypeDef * 形式).

  U8 OriginalTA;

  Cus_CanTP_CanSendFunc SendFunc;         // 底层 CAN 帧发送.(自实现).异步
  Cus_CanTP_CanRecvFunc RecvFunc;         // 底层 CAN 帧接收.
  Cus_CanTP_DataIndication DataIndFunc;   // 上层数据通知回调.
  Cus_CanTP_ErrCallback ErrCallBack;      // 上层错误通知回调.

};


/*  ---------------------------------------------------  */
  #if (USE_CANTP_UTILIS)
    void Cus_Cantp_utilsRecieve_FROM_ISR( CAN_TypeDef *canDevice, uint8_t FIFOx );
    U8 Cus_Cantp_utilsSendAsync( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc );
  #endif 
/*  ---------------------------------------------------  */

/*  ---------------------------------------------------  */
  #if (API_USE_LEGACY)
    U8 Cus_Cantp_Transmit( U8 channelTabID, U8 ConnIndex, const U8 *data, U32 len, void *canDevice );
    U8 Cus_Cantp_Register_Func_Callback( Cus_CanTP_CanSendFunc SendFunc, Cus_CanTP_DataIndication dataIntFunc, Cus_CanTP_ErrCallback errCallback );
  #endif 
/*  ---------------------------------------------------  */


/*  ---------------------------------------------------  */
  void Cus_Cantp_HeartTick( void );
  void Cus_Cantp_MainFunction( void );
  void Cus_Cantp_ReleaseConn( Cus_CANTp_Conn_t *pConn );
  void Cus_Cantp_SystemInit( void );
  void Cus_Cantp_TxConfirmation( void *CanDevice, U8 mailbox );
  U8 Cus_Cantp_RecieveFrame( const U8 *data, U8 dlc, U32 canid );   // 上层喂帧总API.

  // 创建一个接收连接.
  Cus_CANTp_Conn_t *Cus_Cantp_CreateRxConnection( U8 ownAddr, 
                                                  U8 addrMode, 
                                                  U8 bs, 
                                                  U8 stmin, 
                                                  void *canDevice, 
                                                  U8 *bufferRx, 
                                                  U32 size, 
                                                  Cus_CanTP_CanSendFunc sendFunc, 
                                                  Cus_CanTP_DataIndication dataIncFunc, 
                                                  Cus_CanTP_ErrCallback errCallBack );

  // 创建一个发送连接.
  Cus_CANTp_Conn_t *Cus_Cantp_CreateTxConnection( U8 targetAddr, 
                                                  U8 sourceAddr, 
                                                  U8 addrMode, 
                                                  void *canDevice, 
                                                  Cus_CanTP_CanSendFunc sendFunc, 
                                                  Cus_CanTP_ErrCallback errCallback );

  // 请求一次传输.
  S8 Cus_Cantp_startTransmit( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len );
/*  ---------------------------------------------------  */



#endif // __CUS_CANTP_H__
