#include "Cus_CANTP.h"


static Cus_CANTp_Conn_t ConnPool[MAX_SUPPORT_CONN];

/* **************** Extern *************** */
extern Cus_CANTP_Cfg_t ChannelConfigTable_BackUP[CHANNEL_CONFIG_TABLE_COUNT];
extern Cus_CANTP_Cfg_t ChannelConfigTable[CHANNEL_CONFIG_TABLE_COUNT];
/* **************** Extern *************** */

/* ********************************************** Core API ****************************************************** */
  void Cus_Cantp_SystemInit( void );
  void Cus_Cantp_HeartTick( void );
  void Cus_Cantp_MainFunction( void );
  void Cus_Cantp_TxConfirmation( void *CanDevice, U8 mailbox );
  static void Cus_Cantp_RxIndication( Cus_CANTp_Conn_t *pConn, U32 canId, const U8 *data, U8 dlc );
/* ********************************************** Core API ****************************************************** */

/* ********************************************** Get获取相关API ****************************************************** */
  static Cus_CANTp_Conn_t *Cus_Cantp_GetIdleConn( void );
  static U32 Cus_Cantp_GetCanID( const Cus_CANTP_Cfg_t *pCfg );
  static U32 Cus_Cantp_GetDataLengthFromFF( const U8 *frame );
/* ********************************************** Get获取相关API ****************************************************** */

/* ********************************************** 帧发送API ****************************************************** */
  static U8 Cus_Cantp_SendFirstFrame( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 total_len );
  static U8 Cus_Cantp_SendSingleFrame( Cus_CANTp_Conn_t *pConn, const U8 *data, U8 len );
  static U8 Cus_Cantp_SendFlowControlFrame( Cus_CANTp_Conn_t *pConn, Cus_CANTP_FlowState_t fs );
  static U8 Cus_Cantp_SendNextCF( Cus_CANTp_Conn_t *pConn );
/* ********************************************** 帧发送API ****************************************************** */

/* ********************************************** 帧组装API ****************************************************** */
  static U8 Cus_Cantp_BuildSingleFrame( U8 *Buffer, const U8 *data, U8 len, U8 ChannelTabID );
  static U8 Cus_Cantp_BuildFirstFrame( U8 *Buffer, const U8 *data, U32 total_len, U8 ChannelTabID );
  static U8 Cus_Cantp_BuildConsecutiveFrame( U8 *Buffer, const U8 *data, Cus_CANTp_Conn_t *pConn, U8 *pCopylen );
  static U8 Cus_Cantp_BuildFlowControlFrame( U8 *Buffer, Cus_CANTP_FlowState_t flow_State, Cus_CANTp_Conn_t *pConn );
/* ********************************************** 帧组装API ****************************************************** */

/* ********************************************** 帧处理API ****************************************************** */
  static void Cus_Cantp_ProcessSF(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc);
  static void Cus_Cantp_ProcessFF(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc, U32 canId);
  static void Cus_Cantp_ProcessCF(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc);
  static void Cus_Cantp_ProcessFC(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc);
/* ********************************************** 帧处理API ****************************************************** */

/* ***************************************** 启动发送与接收API(对外可见) ************************************************* */
  U8 Cus_Cantp_RecieveFrame( const U8 *data, U8 dlc, U32 canid );
  S8 Cus_Cantp_startTransmit( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len );
/* ***************************************** 启动发送与接收API(对外可见) ************************************************* */

/* ***************************************** 连接块 & 发送通道配置API(对外可见) ************************************************* */
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


  Cus_CANTp_Conn_t *Cus_Cantp_CreateTxConnection( U8 targetAddr, 
                                                  U8 sourceAddr, 
                                                  U8 addrMode, 
                                                  void *canDevice, 
                                                  Cus_CanTP_CanSendFunc sendFunc, 
                                                  Cus_CanTP_ErrCallback errCallback );
/* ***************************************** 连接块 & 发送通道配置API(对外可见) ************************************************* */

/* ********************************************** 初始化及状态重置API ****************************************************** */
  static void __cus_initial_Conn( Cus_CANTp_Conn_t *pConn );
  static void __cus_reset_conn_rx_state(Cus_CANTp_Conn_t *pConn);
  static void __cus_reset_conn_tx_state(Cus_CANTp_Conn_t *pConn);
/* ********************************************** 初始化及状态重置API ****************************************************** */

/* ********************************************** 工具API ****************************************************** */
  static U8 Cus_Cantp_Register_RecvBuffer( U8 *buffer, U32 buffer_Size, U8 ConnIndex );
  static void Cus_Cantp_Config_ChannelNAI_Info( U8 targetAddr, U8 senderAddr, U8 targetAddr_Type, U8 AE, U8 ChannelIndex );
  static void Cus_Cantp_Config_ChannelMain_Info( U8 addrMode, U8 TxDlc, U32 Function_CanID, U8 ChannelIndex );
  static U16 Cus_Cantp_StminConverted( const U8 Stmin );
  static U8 Cus_Cantp_VerifyIDConn(Cus_CANTp_Conn_t *pConn, U32 canId, const U8 *data);
  void Cus_Cantp_ReleaseConn( Cus_CANTp_Conn_t *pConn );
  static void Cus_Cantp_BackupChannelConfig( void );
  static void Cus_Cantp_RestoreChannekConfig( U8 Index );
/* ********************************************** 工具API ****************************************************** */

/* ********************************************** LEGACY ****************************************************** */
#if (API_USE_LEGACY)
  U8 Cus_Cantp_Transmit( U8 channelTabID, U8 ConnIndex, const U8 *data, U32 len, void *canDevice );
  U8 Cus_Cantp_Register_Func_Callback( Cus_CanTP_CanSendFunc SendFunc, Cus_CanTP_DataIndication dataIntFunc, Cus_CanTP_ErrCallback errCallback );
#endif 
/* ********************************************** LEGACY ****************************************************** */

/* ********************************************** UTILS ****************************************************** */
#if (USE_CANTP_UTILIS)
  void Cus_Cantp_utilsRecieve_FROM_ISR( CAN_TypeDef *canDevice, uint8_t FIFOx );
  U8 Cus_Cantp_utilsSendAsync( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc );
#endif 
/* ********************************************** UTILS ****************************************************** */


/* ************************************************************************************************************** */
/* ********************************************** Core API ****************************************************** */
/* Cantp 系统初始化 */
void Cus_Cantp_SystemInit( void )
{
  /* 初始化连接控制块 */
  for( U8 i = 0; i < MAX_SUPPORT_CONN; i++ ) 
  {
    __cus_initial_Conn(&ConnPool[i]);
    ConnPool[i].ConnIndex = -1;
  }

  /* 备份通道配置. 以便后续恢复 */
  Cus_Cantp_BackupChannelConfig();
}


/* Cus_CANTP 的心跳节拍方法. 请将其放入 SystickHandler 中(或其余定时器 周期性调用). */
void Cus_Cantp_HeartTick( void )
{
  for( U8 i = 0; i < MAX_SUPPORT_CONN; i++ )
  {
    Cus_CANTp_Conn_t *pConn = &ConnPool[i];
    if ( pConn->CurrentState == CONN_IDLE ) continue;

    if ( pConn->Timer_N_Ar > 0 )  pConn->Timer_N_Ar--;
    if ( pConn->Timer_N_As > 0 )  pConn->Timer_N_As--;  
    if ( pConn->Timer_N_Bs > 0 )  pConn->Timer_N_Bs--;
    if ( pConn->Timer_N_Cr > 0 )  pConn->Timer_N_Cr--;

    if ( pConn->Timer_StminDelayOnly > 0 )  pConn->Timer_StminDelayOnly--;
  }
}



void Cus_Cantp_MainFunction( void )
{
  for( U8 i = 0; i < MAX_SUPPORT_CONN; i++ )
  {
    Cus_CANTp_Conn_t *pConn = &ConnPool[i];
    if ( pConn->CurrentState == CONN_IDLE )   continue;

    if ( pConn->Timer_N_Bs == 0 && pConn->CurrentState == CONN_TX_WAIT_FC )
    {
      // 等待流控帧超时.
      if ( pConn->ErrCallBack != NULL )
      {
        // 上层注册了错误回调.调用用户错误处理逻辑.
        // Ps: 上层错误处理回调中，pConn状态由上层管理！底层将不负责释放和状态抹除.
        pConn->ErrCallBack(pConn, CUS_CANTP_ERR_NBS_TIMEOUT);  continue;
      }
      else if ( pConn->ErrCallBack == NULL )
      {
        // 未注册上层错误回调.释放控制块. 本次通信结束.
        Cus_Cantp_ReleaseConn(pConn);   continue;
      }
    }

    if ( pConn->Timer_N_Cr == 0 && ((pConn->CurrentState == CONN_RX_WAIT_CF) || (pConn->CurrentState == CONN_RX_CF)) )
    {
      // （作为接收方）等待发送方连续帧超时.
      if ( pConn->ErrCallBack != NULL )
      {
        pConn->ErrCallBack(pConn, CUS_CANTP_ERR_NCR_TIMEOUT);   continue;
      }
      else if ( pConn->ErrCallBack == NULL )
      {
        Cus_Cantp_ReleaseConn(pConn);   continue;
      }
    }

    if ( pConn->Timer_N_As == 0 )
    {
      // 发送确认超时检测
      if ( (pConn->CurrentState == CONN_TX_FF || pConn->CurrentState == CONN_TX_SF) )
      {
        if ( pConn->ErrCallBack != NULL ) pConn->ErrCallBack(pConn, CUS_CANTP_ERR_NAS_TIMEOUT);
        else Cus_Cantp_ReleaseConn(pConn);
        continue;
      }
    }

    if ( pConn->Timer_N_Ar == 0 && pConn->CurrentState == CONN_TX_FC )
    {
      if ( pConn->ErrCallBack != NULL ) pConn->ErrCallBack(pConn, CUS_CANTP_ERR_NAS_TIMEOUT);
      else Cus_Cantp_ReleaseConn(pConn);
      continue;
    }

    // STmin 延时结束，发送下一帧 CF
    if (pConn->Timer_StminDelayOnly == 0 && pConn->CurrentState == CONN_TX_CF) 
    {
      Cus_Cantp_SendNextCF(pConn);
    }
  }
}



void Cus_Cantp_TxConfirmation( void *CanDevice, U8 mailbox )
{
  for(U8 i = 0; i < MAX_SUPPORT_CONN; i++ )
  {
    Cus_CANTp_Conn_t *pConn = &ConnPool[i];

    if ( pConn->CurrentState == CONN_IDLE )   continue;
    if ( (U8 *)pConn->BindCANDevice != (U8 *)CanDevice )  continue;   // 为了便于移植. 此处不使用HAL提供的 CAN_TypeDef类型. 将其转化为U8 *进行匹配.
    if ( pConn->TxMailBoxIndex == mailbox )
    {
      // 匹配到对应设备.开始更新状态.
      switch (pConn->CurrentState)
      {
      case CONN_TX_FF:
        {
          // 首帧发送成功.
          pConn->CurrentState = CONN_TX_WAIT_FC;    // 转为等待流控.

          // 启动 N_Bs定时器.
          pConn->Timer_N_Bs = TIMER_NBS;

          pConn->Timer_N_As = 0;  // 发送已经确认，不需要再等待超时. 直接清0
          break;
        }

      case CONN_TX_CF:
        {
          // 连续帧发送成功. 发送成功相关数据状态由发送API本地更新. 此处不做更新，仅作状态转换.
          if ( pConn->Remaining == 0 )  pConn->CurrentState = CONN_IDLE;  // 数据发完. 结束掉此次会话.

          if ( pConn->BS > 0 && pConn->RemainingBS == 0 )
          {
            // 已成功发完一个BS块. 等待下一个流控.
            pConn->CurrentState = CONN_TX_WAIT_FC;

            // 启动 N_Bs定时器.
            pConn->Timer_N_Bs = TIMER_NBS;

            pConn->Timer_N_As = 0;
          }
          else 
          {
            // 当前BS块还有盈余. 但是STmin 延时已经在 SendNextCF 中处理. 此处仅确认发送成功，无额外操作. 等待触发下一次发送.
            pConn->Timer_N_As = 0;
          }
          break;
        }

      case CONN_TX_SF:
        {
          // 单帧发送成功. 单帧直接结束会话即可.(IDLE通信状态)
          pConn->Timer_N_As = 0;
          pConn->CurrentState = CONN_IDLE;

          break;
        }

      case CONN_TX_FC:
        {
          // 流控帧确认发送成功. 改变状态为 CONN_RX_WAIT_CF. 等待发送方的后续连续帧.
          pConn->CurrentState = CONN_RX_WAIT_CF;
          pConn->Timer_N_Ar = 0;
          pConn->Timer_N_Cr = TIMER_NCR;      // 启动NCR. 在 CONN_RX_WAIT_CF 状态下 等待CF.
          break;
        }

      default:    return;
      }
    }
  }
}



static void Cus_Cantp_RxIndication( Cus_CANTp_Conn_t *pConn, U32 canId, const U8 *data, U8 dlc )
{
  if ( !data || dlc < 8 || !pConn 
        || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT 
          || pConn->ConnIndex >= MAX_SUPPORT_CONN )   return;   // 由于填充机制. 所有小于8的帧都将自动填充到8字节. 因此dlc必须 >= 8.

  if ( dlc == 8 )
  {
    // 经典CAN处理方式. 非CAN FD.
    Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
    if ( !pCfg )  return;

    U8 pciIndex = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 0 : 1;
    U8 byte0_high4Bit = (data[pciIndex] & 0xF0); // 取出Byte0 高四位(帧类型识别码).
    if ( byte0_high4Bit == 0x00 )  
    {
      pConn->CurrentState = CONN_RX_SF;
      Cus_Cantp_ProcessSF(pConn, data, dlc);   // SF帧 转发给SF处理机制.
    }

    if ( byte0_high4Bit == 0x10 )
    {
      pConn->CurrentState = CONN_RX_FF;
      Cus_Cantp_ProcessFF(pConn, data, dlc, canId);   // FF帧 转发给FF处理机制.
    }

    if ( byte0_high4Bit == 0x20 )
    {
      pConn->CurrentState = CONN_RX_CF;
      Cus_Cantp_ProcessCF(pConn, data, dlc);    // CF帧 转发给CF处理机制.
    }

    if ( byte0_high4Bit == 0x30 )
    {
      Cus_Cantp_ProcessFC(pConn, data, dlc);    // FC帧 转发给流控处理.
    }
  }
  else if ( dlc > 8 )
  {
    // CAN FD处理方式.
  }
}
/* ********************************************** Core API ****************************************************** */
/* ************************************************************************************************************** */



/* ****************************************************************************************************************** */
/* ********************************************** Get获取相关API ****************************************************** */
/* 获取空闲连接控制块 */
static Cus_CANTp_Conn_t *Cus_Cantp_GetIdleConn( void )
{
  for( U8 i = 0; i < MAX_SUPPORT_CONN; i++ )
  {
    if ( ConnPool[i].ConnIndex == -1 && ConnPool[i].CurrentState == CONN_IDLE )    // 找到空闲块. 返回地址.
    { 
      /* ReleaseConn() 只将 CurrentState 和 ConnIndex 改为 -1，空闲连接块重新初始化（字段清0）是在此处清0! */ 
      __cus_initial_Conn( &ConnPool[i] );

      /* 重置该连接对应的通道配置表项 */
      Cus_Cantp_RestoreChannekConfig(i);

      ConnPool[i].ConnIndex = i;
      return &ConnPool[i];
    }
  }

  return NULL;        // 所以通道都有活跃会话. 返回NULL.
}



static U32 Cus_Cantp_GetCanID( const Cus_CANTP_Cfg_t *pCfg )
{
  if ( !pCfg )    return 0;

  if ( pCfg->N_AI.TA_Type != 0 && pCfg->N_AI.TA_Type != 1 )   return 0;   // TA_Type参数错误.

  if ( pCfg->N_AI.TA_Type != 0 )  return pCfg->FunctionalCanID;           // 地址类型为功能寻址. 直接返回FunctionalID.

  if ( pCfg->AddrMode == NORMAL_ADDRESS_MODE )
  {
    // 普通寻址模式. N_AI映射到 CANID 段.
    // 0位为 TA_Type. 1~6位为 TA. 7 ~ 11位为 SA.共组成11位标准CANID.
    U32 RealCANID = (((U32)pCfg->N_AI.SA & 0x1FUL) << 6 ) | (((U32)pCfg->N_AI.TA & 0x1FUL) << 1) | ((U32)pCfg->N_AI.TA_Type & 0x01UL);
    return RealCANID & 0x7FFUL;     // 确保地址不超过11位范围.
  }

  if ( pCfg->AddrMode == EXT_ADDRESS_MODE )
  {
    // 拓展寻址模式. 8位TA移入数据段. 剩余8位SA + 1位TAType.
    // 11位ID分配: SA(8位) + TAType(1位) + 2填充(填充为0).
    // 拓展寻址. 待实现.
    U32 RealCANID = ((U32)pCfg->N_AI.SA << 3) | ((U32)(pCfg->N_AI.TA_Type & 0x01UL) << 2) & 0xFCUL; 
    RealCANID &= 0x7FFUL;

    return RealCANID;
  }

  if ( pCfg->AddrMode == MIXED_ADDRESS_MODE )
  {
    // 混合寻址. 待实现.
    return 0;
  }

  return 0;
}



static U32 Cus_Cantp_GetDataLengthFromFF( const U8 *frame )
{
  if ( !frame )   return 0;

  U8 byte0 = frame[0];
  U8 byte1 = frame[1];

  if ( (byte0 >> 4) != 0x01 )   return 0;         // 帧识别码错误. 该帧不是FF帧.

  if ( (byte0 == (0x01 << 4)) && (byte1 == 0) )   // 超长数据包形式.( > 4095 )
  {
    // 扩展格式：4 字节大端长度
    return ((U32)frame[2] << 24) |
    ((U32)frame[3] << 16) |
    ((U32)frame[4] << 8)  |
    (U32)frame[5];
  }

  return  (U32)(((U16)(byte0 & 0x0F) << 8) | (U16)byte1); 
}
/* ********************************************** Get获取相关API ****************************************************** */
/* ****************************************************************************************************************** */



/* ************************************************************************************************************** */
/* ********************************************** 帧发送API ****************************************************** */
static U8 Cus_Cantp_SendFirstFrame( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 total_len )
{
  if ( !pConn || !data || total_len == 0 || 
              pConn->ConnIndex >= MAX_SUPPORT_CONN || pConn->ConnIndex < 0 ||
                pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT )   return 0;   // pConn的配置无效. 返回.

  if ( pConn->CurrentState != CONN_IDLE )   return 0;     // 连接正忙. 返回.

  if ( pConn->SendFunc == NULL )   return 0;          // 发送回调未被注册. 返回.

  // 获取该连接块所属的发送通道.
  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel( pConn->ChannelConfigTabID );    
  if ( !pCfg )    return 0;

  // 构建首帧.
  U8 Buffer[8] = { 0 };
  U8 Return = Cus_Cantp_BuildFirstFrame( Buffer, data, total_len, pConn->ChannelConfigTabID );
  if ( Return != 8 )  return 0;   // 注意此处的返回值检查. 目前只检查了经典CAN情况. 仍未适配CANFD情况，注意.   

  // 计算要发送的CAN报文ID.
  U32 canid = Cus_Cantp_GetCanID(pCfg);
  if ( canid == 0 )   return 0;  

  pConn->CurrentState = CONN_TX_FF;   // 改变状态 进入发送操作.等待底层发送确认.

  // 将TA切换为本机地址(用于接收流控帧).
  pCfg->N_AI.TA = pCfg->N_AI.SA;

  if ( pConn->SendFunc(pConn, canid, Buffer, 8) != 1 )
  {
    // 发送失败. 
    pConn->CurrentState = CONN_IDLE;

    // 将备份的目标地址恢复.
    pCfg->N_AI.TA = pConn->OriginalTA;   
    return 0;
  }
  pConn->Timer_N_As = TIMER_NAS;   // 启动发送确认超时

  U8 FF_payload = 0;
  if ( pCfg->TxDLC == 8 && total_len <= 4095 )  
  {
    FF_payload = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 6 : 5; // 常规长度数据包.根据寻址模式选择携带用户数据包大小
  }
  if ( pCfg->TxDLC == 8 && total_len > 4095 )
  {
    FF_payload = 0;   // 超长包首帧不携带任何用户数据.
  }

  // 首帧发送成功. 修改状态.
  pConn->TotalSize = total_len;
  pConn->TxBytes = FF_payload;
  pConn->pSendData = data;    // 记录下当前发送数据源指针.
  pConn->Remaining = total_len - pConn->TxBytes;   
  pConn->TxPos = pConn->TxBytes;    // 由于发送数据的索引从0开始. 因此TxPos就等于TxBytes. 无需额外加1.
  pConn->SN_Code = (pConn->SN_Code + 1) & 0x0F;   // 首帧 SN 为0 后面首个CF SN为1.(模16循环. FF一般情况不会出现溢出. 此处仅作统一).

  return 1;
}



static U8 Cus_Cantp_SendSingleFrame( Cus_CANTp_Conn_t *pConn, const U8 *data, U8 len )
{
  if ( !pConn || !data || pConn->ConnIndex >= MAX_SUPPORT_CONN || 
            pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT )   return 0;

  if ( pConn->CurrentState != CONN_IDLE )   return 0;     // 忙状态.
  
  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )   return 0;

  // 实际上，此处已由上层判断该发送单帧. 此处再次进行审查.
  U8 ValidMaxlen;
  ValidMaxlen = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 7 : 6;  // EXT寻址模式下. SF帧经典CAN能承载的用户数据段为6个字节.
  if ( len > ValidMaxlen )    return 0;       // 单帧无法容纳. 返回.

  U8 buffer[8];
  U8 ret_dlc = Cus_Cantp_BuildSingleFrame(buffer, data, len, pConn->ChannelConfigTabID);
  if ( ret_dlc == 0 )   return 0;

  U32 Canid;
  Canid = Cus_Cantp_GetCanID(pCfg);
  if ( Canid == 0 )   return 0;
  
  if ( pConn->SendFunc && pConn->SendFunc(pConn, Canid, buffer, ret_dlc) != 0 )
  {
    // 发送成功.
    pConn->Timer_N_As = TIMER_NAS;   // 启动发送确认超时
    pConn->CurrentState = CONN_TX_SF;      // 状态变化为发送单帧. 等待确认.
    return 1;
  }

  return 0;
}



static U8 Cus_Cantp_SendFlowControlFrame( Cus_CANTp_Conn_t *pConn, Cus_CANTP_FlowState_t fs )
{
  if ( !pConn || pConn->ConnIndex >= MAX_SUPPORT_CONN || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT )    return 0;

  if ( pConn->CurrentState != CONN_RX_FF
      && pConn->CurrentState != CONN_RX_WAIT_CF
      && pConn->CurrentState != CONN_RX_CF )   return 0;    // 不处于接收流程中. 返回.

  U8 Buffer[8];
  U8 ret_dlc = Cus_Cantp_BuildFlowControlFrame(Buffer, fs, pConn);
  if ( ret_dlc == 0 )   return 0;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  return 0;
  U32 Canid = Cus_Cantp_GetCanID(pCfg);
  if ( Canid == 0 )   return 0;

  if ( pConn->SendFunc && pConn->SendFunc(pConn, Canid, Buffer, ret_dlc) != 0 )
  {
    // 发送请求提交完成.
    if ( fs == FLOW_CTS )
    {
      // 更新状态. 启动N_Cr定时器.
      pConn->CurrentState = CONN_TX_FC;
      pConn->RemainingBS = pConn->BS;
      pConn->Timer_N_Ar = TIMER_NAR;   // 启动发送确认超时
    }
    else if ( fs == FLOW_OVFLW || fs == FLOW_WAIT )
    {
      // 传输中止. 此处为了简化，将FLOW_WAIT状态并入FLOW_OVFLW进行处理.
      pConn->CurrentState = CONN_IDLE;
    }
  }

  return 1;
}



static U8 Cus_Cantp_SendNextCF( Cus_CANTp_Conn_t *pConn )
{
  if ( !pConn || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT 
               || pConn->ConnIndex >= MAX_SUPPORT_CONN )  return 0;

  if ( pConn->CurrentState != CONN_TX_CF )  return 0;   // 状态错误 返回.

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  
  {
    pConn->CurrentState = CONN_IDLE;
    return 0;
  }

  // 已经没有数据了. 在 TxConfirmation 中会处理该情况.按理来说当 Remaining == 0 时状态已经转化，不会再进入 Cus_Cantp_SendNextCF.
  // 此处仅作防御性处理.
  if ( pConn->Remaining == 0 )  return 0;   

  U8 buffer[8] = { 0 };
  U8 copylen;
  U8 dlc = Cus_Cantp_BuildConsecutiveFrame(buffer, pConn->pSendData, pConn, &copylen);
  if ( dlc != 8 )   // 注意，目前帧构建的返回值检查均只针对经典CAN情况做了适配. 仍未适配CAN FD!
  {
    pConn->CurrentState = CONN_IDLE;
    return 0;
  }

  U32 canID = Cus_Cantp_GetCanID(pCfg);
  if ( !canID )
  {
    pConn->CurrentState = CONN_IDLE;
    return 0;
  }

  if ( pConn->SendFunc && pConn->SendFunc(pConn, canID, buffer, dlc) )
  {
    pConn->Timer_N_As = TIMER_NAS;   // 启动发送确认超时
    // 更新发送状态.  
    // Ps: 此处是提交发送后直接进行状态更新,而并非是等底层硬件发送成功. 因此必须保证 用户自身注册的SendFunc在无邮箱可用时立即返回失败(或有对应重试机制)，而不应等待邮箱变空.
    #warning "SendFunc must return failure immediately when no Tx mailbox available.do NOT block waiting."
    pConn->TxBytes +=  copylen;
    pConn->TxPos += copylen;
    pConn->Remaining -= copylen;
    pConn->SN_Code = (pConn->SN_Code + 1) & 0x0F;   // 模16循环.
    if ( pConn->BS > 0 && pConn->RemainingBS != 0 )   pConn->RemainingBS--;  

    if ( pConn->Remaining > 0 && (pConn->BS == 0 || pConn->RemainingBS > 0) )
    {
      // 还有数据且未达到块限制. 开启Stmin定时器.开启 N_As 定时器.
      if ( pConn->STmin > 0 )
      {
        pConn->Timer_StminDelayOnly = Cus_Cantp_StminConverted(pConn->STmin);
      }
    }
  }
  else 
  {
    // 发送失败.
    pConn->CurrentState = CONN_IDLE;
    return 0;
  }

  return 1;
}
/* ********************************************** 帧发送API ****************************************************** */
/* ************************************************************************************************************** */



/* ************************************************************************************************************** */
/* ********************************************** 帧组装API ****************************************************** */
static U8 Cus_Cantp_BuildSingleFrame( U8 *Buffer, const U8 *data, U8 len, U8 ChannelTabID )
{
  if ( !Buffer || !data || len == 0 || ChannelTabID >= CHANNEL_CONFIG_TABLE_COUNT )  return 0;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(ChannelTabID);
  if ( !pCfg )  return 0;

  U8 maxDataLen = 0;
  U8 pciOffset = 0;
  U8 BufferPos = 0;

  switch (pCfg->AddrMode)
  {
  case NORMAL_ADDRESS_MODE:   // 普通模式.
    {
      if ( pCfg->TxDLC == 8 ) 
      {
        maxDataLen = 8 - 1;     // 经典CAN为8字节数据段. 如果TxDLC < 8. 依然填充到8字节.
        if ( len > maxDataLen )   return 0;   // 单帧放不下. 返回.   
        Buffer[BufferPos++] = (0x00 | (len & 0x0F));    // PCI 信息块写入.
        for( U8 i = 0; i < len; i++ )
        {
          Buffer[BufferPos] = data[i];
          BufferPos++;
        }
        if ( len < maxDataLen )     // 发送的数据不足一个单帧所能承载的数据.剩余位填充为0.
        {
          for( U8 i = BufferPos; i < pCfg->TxDLC; i++ )
          {
            Buffer[i] = 0xCC;
          }
        }
        return pCfg->TxDLC;     
      }
      break;
    }

  case EXT_ADDRESS_MODE:
    {
      pciOffset = 1;        // 拓展寻址下. 地址信息占一个字节，PCI信息偏移到第二个字节.
      BufferPos += 2;       // 提前移动bufferpos，前面两个字节分别划给 地址信息 和 PCI.
      if ( pCfg->TxDLC == 8 )   // 经典CAN.
      {
        maxDataLen = 8 - 2; // 第一个字节是地址，第二个字节是PCI.
        if ( len > maxDataLen )   return 0;
        // 地址信息写入. 将TA(8位)放入数据场第一个字节.
        Buffer[0] = (pCfg->N_AI.TA & 0xFF);
        Buffer[pciOffset] = (0x00 | (len & 0x0F));  // PCI写入.
        for( U8 i = 0; i < len; i++ )
        {
          Buffer[BufferPos] = data[i];
          BufferPos++;
        }
        if ( len < maxDataLen )
        {
          for( U8 i = BufferPos; i < pCfg->TxDLC; i++ )
          {
            Buffer[i] = 0xCC;
            BufferPos++;    // 为了统一. 此处也作自增.
          }
        }
        return pCfg->TxDLC;
      }
      break;
    }
  
  default:
    break;
  }

  return 0;
}



static U8 Cus_Cantp_BuildFirstFrame( U8 *Buffer, const U8 *data, U32 total_len, U8 ChannelTabID )
{
  if ( !Buffer || !data || ChannelTabID >= CHANNEL_CONFIG_TABLE_COUNT )   return 0;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel( ChannelTabID );
  if ( !pCfg )  return 0;

  if ( ( pCfg->TxDLC == 8 && total_len <= 7 && pCfg->AddrMode == NORMAL_ADDRESS_MODE )
        || ( pCfg->TxDLC == 8 && total_len <= 6 && pCfg->AddrMode != NORMAL_ADDRESS_MODE ) )  return 0;   // 经典CAN情况. 可以走单帧发送. 不构造FF帧.

  if ( (pCfg->TxDLC > 8 && total_len <= pCfg->TxDLC - 1) )    return 0;   // CAN FD情况 同样一帧就能发完. 走单帧发送，不构造FF帧.

  U8 maxDataLen = 0;
  U8 BufferPos = 0;
  U8 pciOffset = 0;

  switch (pCfg->AddrMode)
  {
  case NORMAL_ADDRESS_MODE:
    {
      pciOffset = 0;        // 普通寻址模式. PCI无偏移.
      if ( pCfg->TxDLC == 8 && total_len <= 4095 )   // 经典CAN情况.
      {
        maxDataLen = 8 - 2;     // FF PCI占2字节.
        U8 byte0_high4Bit = (0x01 << 4) & 0xF0;
        U8 byte0_low4Bit = (total_len >> 8) & 0x0F;  // FF_DL的高四位.
        Buffer[0] = byte0_high4Bit | byte0_low4Bit;   // 写入PCI第一个字节.
        BufferPos++;

        U8 byte1_8bit = (total_len & 0xFF);   // 写入PCI第二个字节.
        Buffer[1] = byte1_8bit;
        BufferPos++;

        for( U8 i = 0; i < maxDataLen; i++ )    // 剩余还能带6个字节用户数据.
        {
          Buffer[BufferPos] = data[i];
          BufferPos++;
        }
        return pCfg->TxDLC;
      }

      if ( pCfg->TxDLC == 8 && total_len > 4095 )   // 超长数据包.
      {
        maxDataLen = 0;     // 4字节 FF_DL, 2字节 PCI. 剩余字节不用作负载用户数据.
        Buffer[BufferPos++] = (0x01UL << 4);  
        Buffer[BufferPos++] = (0x00);    

        // 按大端字节序 放入FF_DL.
        Buffer[BufferPos++] = (total_len >> 24) & 0xFF;
        Buffer[BufferPos++] = (total_len >> 16) & 0xFF;
        Buffer[BufferPos++] = (total_len >> 8) & 0xFF;
        Buffer[BufferPos++] = total_len & 0xFF;

        Buffer[BufferPos++] = 0xCC;   // 剩余两字节不携带用户数据 默认填充.
        Buffer[BufferPos++] = 0xCC;

        return pCfg->TxDLC;
      }

      break;
    }

  default:
    break;
  }

  return 0;
}



static U8 Cus_Cantp_BuildConsecutiveFrame( U8 *Buffer, const U8 *data, Cus_CANTp_Conn_t *pConn, U8 *pCopylen )
{
  if ( !Buffer || !data || !pConn || pConn->ConnIndex < 0 || !pCopylen )   return 0;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  return 0;

  if ( pCfg->TxDLC != 8 )   return 0;     // 该 API 为经典CAN 相关API. 使用 CANFD，请调用 Cus_Cantp_BuildConsecutiveFrame_CanFD.

  U8 pciOffset = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 1 : 2;
  U8 BufferPos = 0;
  U8 maxDataLen = 8 - pciOffset;

  if ( pCfg->AddrMode == EXT_ADDRESS_MODE )
  {
    Buffer[BufferPos++] = pCfg->N_AI.TA;  // 拓展寻址模式. 将TA写进第一个字节.
  }

  U8 byte0_high4Bit = (((0x01 << 1) << 4) & 0xF0);   // CF帧类型识别码.
  U8 byte0_low4Bit = (pConn->SN_Code & 0x0FUL);   // SN.

  Buffer[BufferPos++] = byte0_high4Bit | byte0_low4Bit;

  U8 copylen = (pConn->Remaining <= maxDataLen) ? pConn->Remaining : maxDataLen;
  *pCopylen = copylen;
  for( U8 i = 0; i < copylen; i++ )
  {
    Buffer[BufferPos++] = data[(pConn->TxPos) + i];   // 注意！此处只使用 Cus_CANTp_Conn_t 的数据，而不更新状态！ 所有状态均在发送/接收API中统一更新.
  }

  if ( copylen != maxDataLen )  
  {                               // 用默认值填补到一个标准8字节数据段.
    while ( BufferPos < 8 ) 
    {
      Buffer[BufferPos++] = 0xCC;
    }
  }

  if ( BufferPos != 8 )       // BufferPos 错位. 帧构造内部可能已经出现未知错乱. 清空帧后退出.(极低概率).
  {
    memset(Buffer, 0xCC, 8);
    return 0;
  }

  return pCfg->TxDLC;
}



static U8 Cus_Cantp_BuildFlowControlFrame( U8 *Buffer, Cus_CANTP_FlowState_t flow_State, Cus_CANTp_Conn_t *pConn )
{
  if ( !Buffer || !pConn || pConn->ConnIndex < 0 )  return 0;

  U8 BufferPos = 0;
  U8 byte0_high4Bit = (((0x03UL) << 4) & 0xF0UL );
  U8 byte0_low4Bit = 0;
  switch (flow_State)
  {
    case FLOW_CTS:  byte0_low4Bit = 0x00;   break;
    case FLOW_WAIT: byte0_low4Bit = (0x01 & 0x0F); break;
    case FLOW_OVFLW:  byte0_low4Bit = ((0x01 << 1) & 0x0F); break;
  
    default:  return 0; 
  }

  Buffer[BufferPos++] = byte0_high4Bit | byte0_low4Bit;
  Buffer[BufferPos++] = (pConn->BS & 0xFF);
  Buffer[BufferPos++] = (pConn->STmin & 0xFF);

  while( BufferPos < 8 )
  {
    // FC帧不携带用户数据. 剩余字节全部默认填充.
    Buffer[BufferPos++] = 0xCC;
  }

  return Cus_Cantp_GetChannel(pConn->ChannelConfigTabID)->TxDLC;
}
/* ********************************************** 帧组装API ****************************************************** */
/* ************************************************************************************************************** */



/* ************************************************************************************************************** */
/* ********************************************** 帧处理API ****************************************************** */
static void Cus_Cantp_ProcessSF(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc)
{
  if ( !pConn || pConn->ConnIndex >= MAX_SUPPORT_CONN || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT )  return;

  if ( pConn->CurrentState != CONN_RX_SF )  return;   // 不处于 CONN_RX_SF 状态. 返回.

  (void)dlc;    // 在不实现 CAN FD的情况下，dlc参数冗余. 此处仅为后续可能的拓展留出接口.

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  return;

  U8 sf_len;
  const U8 *payload = NULL;
  switch (pCfg->AddrMode)
  {
  case NORMAL_ADDRESS_MODE:   
    {
      sf_len = (data[0] & 0x0F);  
      if ( sf_len == 0 || sf_len > 7 )  return;     // 无效长度 直接返回.
      payload = &data[1];   // PCI 信息字节在 Byte0.
      pConn->CurrentState = CONN_IDLE;              // SF帧通信结束. 控制块回到IDLE状态.
      pConn->DataIndFunc(pConn, payload, sf_len);   // 通知上层取数据.
      __cus_reset_conn_rx_state(pConn);             // SF 单帧通信结束. 打扫控制块状态.
      break;
    }
  
  case EXT_ADDRESS_MODE:
    {
      sf_len = (data[1] & 0x0F);      // EXT模式下. 数据段中 Byte0存的是TA. 因此SF_DL从Byte1开始.
      if ( sf_len == 0 || sf_len > 6 )  return;     // 无效长度 返回.
      payload = &data[2];   // Byte0是 地址信息. Byte1是 PCI信息. 数据段从Byte2开始.
      pConn->CurrentState = CONN_IDLE;
      pConn->DataIndFunc(pConn, payload, sf_len);   
      __cus_reset_conn_rx_state(pConn);
      break;
    }
  default:  return;
  }
}



static void Cus_Cantp_ProcessFF(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc, U32 canId)
{
  if ( !pConn || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT 
               || pConn->ConnIndex >= MAX_SUPPORT_CONN || !data )     return;

  if ( pConn->CurrentState != CONN_RX_FF )    return;   // 状态不正确. 返回.

  (void)dlc;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  
  {
    pConn->CurrentState = CONN_IDLE; 
    return;
  }

  // 从接收到的ID中解析接收方地址.
  U8 Sender_SA = 0;
  if ( pCfg->AddrMode == NORMAL_ADDRESS_MODE )
  {
    Sender_SA = (canId >> 6) & 0x1F;    // SA 位于 bits 10:6.
  }
  else if ( pCfg->AddrMode == EXT_ADDRESS_MODE )
  {
    Sender_SA = (canId >> 3) & 0xFF;    // SA 位于 bits 10:3. 
  }

  // 临时改为发送方的 SA. 发送流控.
  pCfg->N_AI.TA = Sender_SA;    // FC 的 TA 必须是发送方地址.

  U32 total_length = Cus_Cantp_GetDataLengthFromFF(data);
  if ( !total_length ) 
  {
    pConn->CurrentState = CONN_IDLE; 
    pCfg->N_AI.TA = pConn->OriginalTA;
    return;
  } 

  if ( !pConn->pRecvBuffer || pConn->RecvBuffer_Size < total_length )
  {
    // 缓冲区不足. 发送OVFLW. 并结束掉通信.
    Cus_Cantp_SendFlowControlFrame(pConn, FLOW_OVFLW);
    pCfg->N_AI.TA = pConn->OriginalTA;
    pConn->CurrentState = CONN_IDLE;    // 通信已经结束.恢复IDLE状态.
    return;
  }

  U8 ff_payload;
  const U8 *payload = NULL;
  if ( total_length <= 4095 )
  {
    ff_payload = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 6 : 5;   // 经典CAN下 根据寻址模式不同,携带 6/5 字节负载.
    payload = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? &data[2] : &data[3];
  }
  else if ( total_length > 4095 )
  {
    // 超长数据包形式.
    ff_payload = 0;   // 超长包形式不携带用户数据负载.
  }

  if ( payload != NULL )
  {
    // 将FF帧中数据拷贝入缓冲区后. 更新状态.
    memcpy(pConn->pRecvBuffer, payload, ff_payload);
  }
  pConn->TotalSize = total_length;
  pConn->RxBytes = ff_payload;
  pConn->RxPos = ff_payload;
  pConn->SN_Code = (pConn->SN_Code + 1) & 0x0F;     // 接收到FF的SN为0. 下一帧期望的CF SNCODE应该为1.
  pConn->Remaining = pConn->TotalSize - pConn->RxBytes;

  Cus_Cantp_SendFlowControlFrame(pConn, FLOW_CTS);    // 发送流控.(内部置起 CONN_RX_WAIT_CF)

  // 恢复本机地址.
  pCfg->N_AI.TA = pConn->OriginalTA;
}



static void Cus_Cantp_ProcessCF(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc)
{
  if ( !pConn || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT 
               || pConn->ConnIndex >= MAX_SUPPORT_CONN || !data )    return;

  if ( pConn->CurrentState != CONN_RX_CF )    return;     // 状态不正确.
  (void) dlc;           // 冗余参数. 为后续留出接口.

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  
  {
    pConn->CurrentState = CONN_IDLE;
    return;
  }

  U8 pciOffset = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ?  1 : 2;
  U8 SnCode;
  if ( pciOffset == 1 )
  {
    SnCode = (data[0] & 0x0F);     // 普通寻址模式. PCI在第一个字节.获取低四位的SN码.
  }
  else if ( pciOffset == 2 )
  {
    SnCode = (data[1] & 0x0F);    // 拓展寻址模式. PCI在第2个字节.获取低四位的SN码.
  }
  else 
  {
    pConn->CurrentState = CONN_IDLE;
    return;
  }

  if ( pConn->SN_Code != SnCode )   // SN校验出错. 当前帧SN码与接收机期望SN码不相同. 中止接收，更新状态返回.
  {
    Cus_Cantp_SendFlowControlFrame(pConn, FLOW_OVFLW);    // 通知发送方中止发送. 内部置 IDLE 状态.
    __cus_reset_conn_rx_state(pConn);     // 重置接收状态.     

    return;
  }

  // SN校验成功. 接收该帧数据.
  const U8 *pData = &data[pciOffset];
  U8 data_len = 8 - pciOffset;    // 除去PCI（可能的TA）后,剩余数据段大小.
  U8 copylen = ( pConn->Remaining <= data_len ) ? (pConn->Remaining) : data_len;
  memcpy(&pConn->pRecvBuffer[pConn->RxPos], pData, copylen);

  // 更新接收后状态.
  pConn->RxBytes += copylen;
  pConn->RxPos += copylen;
  pConn->Remaining -= copylen;
  pConn->SN_Code = (pConn->SN_Code + 1) & 0x0F;   // 下一帧期望的SNCODE.

  if ( pConn->BS > 0 && pConn->RemainingBS > 0 )  pConn->RemainingBS--;   // 块计数器处理.

  // 判断是否接收完成.
  if ( pConn->Remaining == 0 )
  {
    // 所有数据已接收完毕. 通知上层.
    if ( pConn->DataIndFunc )
    {
      pConn->DataIndFunc(pConn, pConn->pRecvBuffer, pConn->RxBytes);
    }
    pConn->CurrentState = CONN_IDLE;
    __cus_reset_conn_rx_state(pConn);
    return;
  }

  // 未接收完毕.
  if ( pConn->BS > 0 && pConn->RemainingBS == 0 )
  {
    // 需要向接收方发送新的流控帧.
    // Ps: Cus_Cantp_SendFlowControlFrame 内部会设置 CONN_RX_WAIT_CF状态 以及重新启动 Ncr 定时器.
    Cus_Cantp_SendFlowControlFrame(pConn, FLOW_CTS);    // 向接收方发送新的FC来允许下一个块.
  }
  else 
  {
    // 单个CF接收完毕. 仍在同一块内，重置定时器.继续等待下一CF.
    pConn->Timer_N_Cr = TIMER_NCR;
  }
}



static void Cus_Cantp_ProcessFC(Cus_CANTp_Conn_t *pConn, const U8 *data, U8 dlc)
{
  if ( !pConn || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT 
               || pConn->ConnIndex >= MAX_SUPPORT_CONN || !data )   return;

  (void) dlc;   // FC帧不携带用户数据. 所以无论是经典CAN还是CAN FD处理方式是一样的. 此处dlc参数仅作为预留拓展接口占位.

  if ( pConn->CurrentState != CONN_TX_WAIT_FC )   return;   // 状态不对返回. 接收流控只针对作为发送方(Sender)而言.

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  return;

  U8 pciOffset = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 1 : 2;
  S8 FlowState = -1;
  if ( pciOffset == 1 )
  {
    // 普通寻址模式 PCI信息字节在第一个字节.
    FlowState = (data[0] & 0x0F);
  }
  else if ( pciOffset == 2 )
  {
    // 拓展寻址模式 PCI信息字节在第二个字节.
    FlowState = (data[1] & 0x0F); 
  } 
  else return;

  pConn->BS = data[pciOffset];    // 取出 流控帧 中携带的BS.
  pConn->RemainingBS = (pConn->BS == 0) ? 0xFFFF : pConn->BS;   
  pConn->STmin = data[pciOffset + 1];   // 取出 流控帧 中携带的STmin.

  switch ((Cus_CANTP_FlowState_t)FlowState)
  {
  case FLOW_CTS:  pConn->CurrentState = CONN_TX_CF; pCfg->N_AI.TA = pConn->OriginalTA; break;   // 转换状态. 开启CF发送. 

  case FLOW_OVFLW: pConn->CurrentState = CONN_IDLE; break;    // 中止传输.

  case FLOW_WAIT: pConn->CurrentState = CONN_IDLE;  break;    // 此处为简化处理，暂时将WAIT与OVFLW合并处理.
  
  default:    return;
  }
}
/* ********************************************** 帧处理API ****************************************************** */
/* ************************************************************************************************************** */



/* ******************************************************************************************************************* */
/* ***************************************** 启动发送与接收API(对外可见) ************************************************* */
U8 Cus_Cantp_RecieveFrame( const U8 *data, U8 dlc, U32 canid )
{
  if ( !data || dlc < 8 || canid == 0 )   return 0;   // 由于CANTP中间层采用填充机制. 因此帧构造时，数据场不足8字节的均会填补到8字节. dlc不足8字节的CAN帧不符合协议规定，忽略.

  U8 handled = 0;

  // 对该帧进行连接分配. 
  for( U8 i = 0; i < MAX_SUPPORT_CONN; i++ )
  {
    Cus_CANTp_Conn_t *pConn = &ConnPool[i];
    Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
    if ( !pCfg )  continue;

    if ( pConn->ConnIndex < 0 )   continue;     // 该连接控制块为未分配状态.

    if ( pCfg->N_AI.TA_Type == 1 )      // 功能寻址模式. 将该帧向每个使用功能寻址的节点进行分发.
    {
      if ( canid != pCfg->FunctionalCanID ) continue;   // CANID不匹配功能地址. 跳过.

      if ( pCfg->AddrMode == EXT_ADDRESS_MODE )
      {
        // 对于拓展寻址模式下. 还需要检查数据段第一个字节是否为TA.
        if ( data[0] != pCfg->N_AI.TA )  continue;
      }

      Cus_Cantp_RxIndication(pConn, canid, data, dlc);
      handled++;
      continue;      // 功能寻址可能多个连接同时匹配，不能 break，继续遍历.
    }

    // 检查连接是否匹配.
    if ( Cus_Cantp_VerifyIDConn(pConn, canid, data) && pCfg->N_AI.TA_Type != 1 )
    {
      // 连接匹配. 调用底层喂帧
      Cus_Cantp_RxIndication(pConn, canid, data, dlc);
      handled++;
      continue;     // 同一 TA 可能对应有多个连接. 因此此处不直接返回.
    }
  }

  return handled;
}



S8 Cus_Cantp_startTransmit( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len )
{
  if ( !pConn || !data || len == 0 )   return -1;

  // 获取连接对应的通道配置表.
  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )   return -2;

  U8 maxSingleLen = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 7 : 6;
  if ( len <= maxSingleLen )
  {
    // 单帧直接发完.
    return Cus_Cantp_SendSingleFrame(pConn, data, len);
  }
  else 
  {
    // 数据较长. 启动多帧传输流程.
    return Cus_Cantp_SendFirstFrame(pConn, data, len);
  }
}

/* ***************************************** 启动发送与接收API(对外可见) ************************************************* */
/* ******************************************************************************************************************* */



/* ************************************************************************************************************************** */
/* ***************************************** 连接块 & 发送通道配置API(对外可见) ************************************************* */
Cus_CANTp_Conn_t *Cus_Cantp_CreateRxConnection( U8 ownAddr, 
                                                U8 addrMode, 
                                                U8 bs, 
                                                U8 stmin, 
                                                void *canDevice, 
                                                U8 *bufferRx, 
                                                U32 size, 
                                                Cus_CanTP_CanSendFunc sendFunc, 
                                                Cus_CanTP_DataIndication dataIncFunc,
                                                Cus_CanTP_ErrCallback errCallBack )
{
  if ( !canDevice || !bufferRx || size == 0 )   return NULL;

  /* 创建连接块时 发送回调和接收回调是必须的. 不能传入NULL */
  if ( !sendFunc || !dataIncFunc )    return NULL;

  /* 从连接池中获取一个空闲连接 */ 
  Cus_CANTp_Conn_t *pConn = Cus_Cantp_GetIdleConn();
  if ( !pConn )   return NULL;

  /* 采用连接块ID来对应一张通道表.(一一对应) */
  U8 ChannelIndex = pConn->ConnIndex;

  /* 配置通道参数 */
  Cus_Cantp_Config_ChannelMain_Info(addrMode, CLASSIC_CAN_TXDLC, FUNCTIONAL_ID, ChannelIndex);
  Cus_Cantp_Config_ChannelNAI_Info(ownAddr, CANTP_RX_SENDER_ADDR, CANTP_TA_TYPE_PHYSICAL, CANTP_AE_NONE, ChannelIndex);

  /* 缓冲区注册 */ 
  U8 hReturn = Cus_Cantp_Register_RecvBuffer(bufferRx, size, pConn->ConnIndex);
  if ( !hReturn )  
  {
    /* 缓冲区注册失败. 将已分配的连接释放 */ 
    Cus_Cantp_ReleaseConn(pConn);
    return NULL;
  }

  pConn->DataIndFunc = dataIncFunc;
  pConn->SendFunc = sendFunc;

  /* 错误回调(可选). 传入NULL则无错误回调 */
  if ( errCallBack )
  {
    pConn->ErrCallBack = errCallBack;
  }

  pConn->BindCANDevice = canDevice;
  pConn->BS = bs;
  pConn->STmin = stmin;
  pConn->OriginalTA = ownAddr;

  return pConn;
}



Cus_CANTp_Conn_t *Cus_Cantp_CreateTxConnection( U8 targetAddr, 
                                                U8 sourceAddr, 
                                                U8 addrMode, 
                                                void *canDevice, 
                                                Cus_CanTP_CanSendFunc sendFunc, 
                                                Cus_CanTP_ErrCallback errCallback )
{
  if ( !canDevice )   return NULL;

  /* 必须传入发送回调 */
  if ( !sendFunc )    return NULL;

  /* 从池中获取空闲连接 */ 
  Cus_CANTp_Conn_t * pConn = Cus_Cantp_GetIdleConn();
  if ( !pConn )   return NULL;

  /* 获取对应通道ID */ 
  U8 ChannelIndex = pConn->ConnIndex;

  /* 配置通道参数 */ 
  Cus_Cantp_Config_ChannelMain_Info(addrMode, CLASSIC_CAN_TXDLC, FUNCTIONAL_ID, ChannelIndex);
  Cus_Cantp_Config_ChannelNAI_Info(targetAddr, sourceAddr, CANTP_TA_TYPE_PHYSICAL, CANTP_AE_NONE, ChannelIndex);

  pConn->SendFunc = sendFunc;

  /* 注册错误回调.(可选) */ 
  if ( errCallback )
  {
    pConn->ErrCallBack = errCallback;
  }

  pConn->BindCANDevice = canDevice;
  pConn->OriginalTA = targetAddr;

  return pConn;
}
/* ***************************************** 连接块 & 发送通道配置API(对外可见) ************************************************* */
/* ************************************************************************************************************************** */



/* *********************************************************************************************************************** */
/* ********************************************** 初始化及状态重置API ****************************************************** */
static void __cus_initial_Conn( Cus_CANTp_Conn_t *pConn )
{
  if ( !pConn )   return;

  pConn->BS = 0;
  pConn->STmin = 0;

  pConn->RxBytes = 0;
  pConn->TxBytes = 0;
  pConn->Remaining = 0;
  pConn->RemainingBS = 0;
  pConn->SN_Code = 0;
  pConn->RxPos = 0;
  pConn->Timer_StminDelayOnly = 0;
  pConn->pRecvBuffer = NULL;
  pConn->RecvBuffer_Size = 0;
  pConn->ConnIndex = -1;      // 先显示设置为-1. 而后由池进行分配.

  pConn->Timer_N_Ar = 0;
  pConn->Timer_N_As = 0;
  pConn->Timer_N_Bs = 0;
  pConn->Timer_N_Cr = 0;

  pConn->CurrentState = CONN_IDLE;
  pConn->ChannelConfigTabID = 0;
  pConn->SendFunc = NULL;
  pConn->RecvFunc = NULL;
  pConn->ErrCallBack = NULL;
  pConn->DataIndFunc = NULL;
  pConn->TxPos = 0;
  pConn->TxMailBoxIndex = 0xFF;
  pConn->BindCANDevice = NULL;
  pConn->pSendData = NULL;
  pConn->OriginalTA = 0;
}



/* 接收方通信信息状态重置API */
static void __cus_reset_conn_rx_state(Cus_CANTp_Conn_t *pConn)
{
  if ( !pConn )  return;

  pConn->TotalSize = 0;
  pConn->Remaining = 0;
  pConn->RemainingBS = 0;
  pConn->RxBytes = 0;
  pConn->RxPos = 0;
  pConn->SN_Code = 0;
  pConn->Timer_N_Ar = 0;
  pConn->Timer_N_Cr = 0;
}



/* 发送方通信信息状态重置API */
static void __cus_reset_conn_tx_state(Cus_CANTp_Conn_t *pConn)
{
  if ( !pConn )   return;

  pConn->TxBytes = 0;
  pConn->pSendData = NULL;
  pConn->TxMailBoxIndex = 0xFF;
  pConn->RemainingBS = 0;
  pConn->SN_Code = 0;
  pConn->TxPos = 0;
  pConn->Timer_N_As = 0;
  pConn->Timer_N_Bs = 0;
  pConn->Timer_StminDelayOnly = 0;
  pConn->STmin = 0;
  pConn->BS = 0;
  pConn->OriginalTA = 0;
}
/* ********************************************** 初始化及状态重置API ****************************************************** */
/* *********************************************************************************************************************** */



/* ************************************************************************************************************ */
/* ********************************************** 工具API ****************************************************** */
U8 Cus_Cantp_Register_RecvBuffer( U8 *buffer, U32 buffer_Size, U8 ConnIndex )
{
  if ( ConnIndex >= MAX_SUPPORT_CONN )  return 0;
  // 该 API 用于向连接块注册缓冲区.
  if ( !buffer || buffer_Size == 0 )  return 0;

  Cus_CANTp_Conn_t *pConn = &ConnPool[ConnIndex];
  pConn->pRecvBuffer = buffer;
  pConn->RecvBuffer_Size = buffer_Size;

  return 1;
}



void Cus_Cantp_Config_ChannelNAI_Info( U8 targetAddr, U8 senderAddr, U8 targetAddr_Type, U8 AE, U8 ChannelIndex )
{
  if ( ChannelIndex >= CHANNEL_CONFIG_TABLE_COUNT )   return;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(ChannelIndex);
  if ( !pCfg )  return;

  if ( pCfg->AddrMode == NORMAL_ADDRESS_MODE )
  {
    pCfg->N_AI.TA = targetAddr & 0x1F;   // 取低5位
    pCfg->N_AI.SA = senderAddr & 0x1F;

    // 确保 targetAddr_Type 只能为 0(普通寻址) 和 1(功能寻址). 其余情况按默认（普通寻址）处理.
    pCfg->N_AI.TA_Type = (targetAddr_Type == 0 || targetAddr_Type == 1) ? targetAddr_Type : 0;
    pCfg->N_AI.AE = 0;    // 普通寻址用不到 AE.
  } 

  if ( pCfg->AddrMode == EXT_ADDRESS_MODE )
  {
    pCfg->N_AI.TA = targetAddr;   // 拓展寻址模式 8位TA.
    pCfg->N_AI.SA = senderAddr;   // 8位SA.
    pCfg->N_AI.TA_Type = (targetAddr_Type == 0 || targetAddr_Type == 1) ? targetAddr_Type : 0;
    pCfg->N_AI.AE = 0;
  }

  if ( pCfg->AddrMode == MIXED_ADDRESS_MODE )
  {
    // 混合寻址模式. 待实现.
  }

}



void Cus_Cantp_Config_ChannelMain_Info( U8 addrMode, U8 TxDlc, U32 Function_CanID, U8 ChannelIndex )
{
  if ( ChannelIndex >= CHANNEL_CONFIG_TABLE_COUNT )   return;

  Cus_CANTP_Cfg_t * pCfg = Cus_Cantp_GetChannel(ChannelIndex);
  if ( !pCfg )    return;

  pCfg->AddrMode = addrMode == (NORMAL_ADDRESS_MODE || addrMode == EXT_ADDRESS_MODE || addrMode == MIXED_ADDRESS_MODE) 
                      ? addrMode : NORMAL_ADDRESS_MODE;

  pCfg->FunctionalCanID = Function_CanID;

  // TxDLC用于区分 经典CAN 和 CANFD. 由于该协议具有自动填充机制，所以不允许DLC小于8字节! 所有小于8的配置都将会默认配置到8字节.
  pCfg->TxDLC = (TxDlc < 8 || TxDlc > 64) ? 8 : TxDlc;    
}



static U16 Cus_Cantp_StminConverted( const U8 Stmin )
{
  // ISO 15765-2: STmin 值含义
  // 0x00 - 0x7F: 直接为毫秒数
  // 0x80 - 0xF0: 保留
  // 0xF1 - 0xF9: 100us - 900us
  // 0xFA - 0xFF: 保留

  U16 ST_Value = (U16)Stmin;
  if ( ST_Value > 0xFF )  return 0;

  if ( ST_Value <= 0x7F )   return ST_Value;  // 直接返回ms.

  if ( (ST_Value >= 0xF1) && (ST_Value <= 0xF9) )
  {
    return 1;             // 简化处理. 最小延时1ms.
  }

  return 0;
}



static U8 Cus_Cantp_VerifyIDConn( Cus_CANTp_Conn_t *pConn, U32 canId, const U8 *data )
{
  if ( !pConn || pConn->ConnIndex >= MAX_SUPPORT_CONN 
                || pConn->ChannelConfigTabID >= CHANNEL_CONFIG_TABLE_COUNT || !data )   return 0;

  Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(pConn->ChannelConfigTabID);
  if ( !pCfg )  return 0;

  // 对于功能寻址模式. 直接将ID与功能ID(Functional ID匹配).
  if ( pCfg->N_AI.TA_Type == 1 )  
    return (canId == pCfg->FunctionalCanID);

  // 非功能寻址模式. 将接收的报文ID中的TA字段与本机TA进行比较(对于接收方而言，自身连接块中TA字段为本机接收地址).
  if ( pCfg->AddrMode == NORMAL_ADDRESS_MODE )
  {
    // 普通寻址模式.
    U8 expectedTA = pCfg->N_AI.TA & 0x1FUL;  
    U8 receiveTA = (canId >> 1) & 0x1FUL;

    return (expectedTA == receiveTA);
  }

  if ( pCfg->AddrMode == EXT_ADDRESS_MODE )
  {
    // 拓展寻址模式.
    U8 expectedTA = pCfg->N_AI.TA;
    U8 receiveTA = data[0];         // 拓展寻址下. 数据场第一个字节为TA.

    return (expectedTA == receiveTA);
  }

  return 0;
}



void Cus_Cantp_ReleaseConn( Cus_CANTp_Conn_t *pConn )
{
  if ( pConn && pConn->ConnIndex < MAX_SUPPORT_CONN )
  {
    // 此处不清空结构体. 清空并初始化对应通道 将在 Cus_Cantp_GetIdleConn 中进行.
    pConn->CurrentState = CONN_IDLE;
    pConn->ConnIndex = -1;
  }
}



static void Cus_Cantp_BackupChannelConfig( void )
{
  memcpy(ChannelConfigTable_BackUP, ChannelConfigTable, sizeof(ChannelConfigTable));
}


/* 复原由Index参数指定的通道表项 */
static void Cus_Cantp_RestoreChannekConfig( U8 Index )
{
  if ( Index < CHANNEL_CONFIG_TABLE_COUNT )  
  {
    ChannelConfigTable[Index] = ChannelConfigTable_BackUP[Index];
  }
}
/* ********************************************** 工具API ****************************************************** */
/* ************************************************************************************************************ */


/* ************************************************************************************************************ */
/* *********************************************** LEGACY_API ************************************************* */
#if (API_USE_LEGACY)
  U8 Cus_Cantp_Transmit( U8 channelTabID, U8 ConnIndex, const U8 *data, U32 len, void *canDevice )
  {
    if ( channelTabID >= CHANNEL_CONFIG_TABLE_COUNT || ConnIndex >= MAX_SUPPORT_CONN )   return 0;

    if ( !data || !canDevice || len == 0 )    return 0;

    Cus_CANTp_Conn_t *pConn = &ConnPool[ConnIndex];
    if ( pConn->ConnIndex != -1 )   return 0;   // 所选定的连接控制块已经被分配. 返回.

    if ( pConn->ConnIndex == -1 )   
    {
      // 所选定的连接控制块未被分配. 对其进行分配. 为接下来通信做准备.
      pConn->ConnIndex = ConnIndex;
    }
    pConn->BindCANDevice = canDevice;
    pConn->ChannelConfigTabID = channelTabID;

    if ( !pConn->SendFunc || !pConn->DataIndFunc )    
    {
      // 未注册必须的 SendFunc 和 DataIndFunc. 释放连接并返回.(errCallback可以选择进行注册，但是 SendFunc与DataIndFunc 必须注册)
      Cus_Cantp_ReleaseConn(pConn);
      return 0;
    }

    Cus_CANTP_Cfg_t *pCfg = Cus_Cantp_GetChannel(channelTabID);
    if ( !pCfg )  
    {
      Cus_Cantp_ReleaseConn(pConn);
      return 0;
    }

    if ( pCfg->TxDLC == 8 )
    {
      // 经典CAN情况.
      U8 maxSingleLen = (pCfg->AddrMode == NORMAL_ADDRESS_MODE) ? 7 : 6;
      if ( len <= maxSingleLen )
          return Cus_Cantp_SendSingleFrame(pConn, data, len);     // 单帧即可发完.
      else 
          return Cus_Cantp_SendFirstFrame(pConn, data, len);      // 启动多帧发送.
    }
    
    if ( pCfg->TxDLC > 8 )
    {
      // CAN FD情况. 待实现.
    }

    return 0;
  }



  U8 Cus_Cantp_Register_Func_Callback( Cus_CanTP_CanSendFunc SendFunc, Cus_CanTP_DataIndication dataIntFunc, Cus_CanTP_ErrCallback errCallback )
  {
    // 该API用于整体设置 底层发送驱动 ,上层接收通知 以及 错误回调.
    if ( !SendFunc || !errCallback || !dataIntFunc )  return 0;

    for( U8 i = 0; i < MAX_SUPPORT_CONN; i++ )
    {
      Cus_CANTp_Conn_t *pConn = &ConnPool[i];
      pConn->SendFunc = SendFunc;
      pConn->ErrCallBack = errCallback;
      pConn->DataIndFunc = dataIntFunc;
    }

    return 1;
  }
#endif // API_USE_LEGACY
/* *********************************************** LEGACY_API ************************************************* */
/* ************************************************************************************************************ */



/* ******************************************************************************************************* */
/* *********************************************** UTILS ************************************************* */
#if (USE_CANTP_UTILIS)
  void Cus_Cantp_utilsRecieve_FROM_ISR( CAN_TypeDef *canDevice, uint8_t FIFOx )
  {
    uint8_t rxD[8];
    Cus_Can_Frame_t rxFrame;
    int8_t hReturn;
    int8_t frameCount = 0;

    volatile uint32_t *RFxR = (FIFOx == CUS_CAN_UTILS_FIFO0) ? &canDevice->RF0R : &canDevice->RF1R;

    /* 取出当前报文数目 (快照) */
    frameCount = (*RFxR & CAN_RF0R_FMP0_Msk);

    if ( frameCount )
    {
      do 
      {
        /* 处理一帧数据 */
        hReturn = Cus_CAN_Utils_RecevieCANFrame(canDevice, &rxD, sizeof(rxD), &rxFrame, FIFOx);
        if ( hReturn < 0 )   continue;  

        /* 向 CANTP 喂帧 */ 
        Cus_Cantp_RecieveFrame(rxD, rxFrame.DLC, rxFrame.ID);

      } while(--frameCount);
    }
  }


  U8 Cus_Cantp_utilsSendAsync( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc )
  {
    if ( !pConn || !pConn->BindCANDevice || !data || dlc < 8 )   return 0;

    Cus_Can_Frame_t txFrame;
    memset(&txFrame, 0, sizeof(txFrame));

    if ( ((canId >> 11) & 0x1FFFFF) )
    {
      /* 拓展寻址模式 */
      txFrame.ID_Type = CUS_CAN_UTILS_IdTYPE_EXTI;
      txFrame.ID = canId & 0x1FFFFFFF;
    }
    else 
    {
      /* 标准寻址模式 */
      txFrame.ID_Type = CUS_CAN_UTILS_IdTYPE_STD;
      txFrame.ID = canId & 0x7FF;
    }

    txFrame.RTR = 0;
    txFrame.DLC = (uint8_t)(dlc & 0x0F);

    uint8_t txBuffer[dlc];
    memcpy(txBuffer, data, (dlc > 8) ? 8 : dlc);

    uint8_t usedMb;
    uint32_t MailBox;
    int8_t hReturn = Cus_CAN_Utils_SendCANFrame_Async((CAN_TypeDef *)pConn->BindCANDevice, txBuffer, sizeof(txBuffer), &txFrame, CUS_CAN_UTILS_TXMAILBOX_NONE, &usedMb);
    if ( hReturn < 0 )    return 0;

    switch (usedMb)
    {
      case CUS_CAN_UTILS_TXMAILBOX_0:  MailBox = CAN_TX_MAILBOX0;   break;
      case CUS_CAN_UTILS_TXMAILBOX_1:  MailBox = CAN_TX_MAILBOX1;   break;
      case CUS_CAN_UTILS_TXMAILBOX_2:  MailBox = CAN_TX_MAILBOX2;   break;
    
      default:    return 0;
    }

    pConn->TxMailBoxIndex = MailBox;

    return 1;
  }
#endif // USE_CANTP_UTILIS
/* *********************************************** UTILS ************************************************* */
/* ******************************************************************************************************* */


