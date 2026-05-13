/**
 * @file    Cus_CAN_IT.c
 * @brief   CAN 中断处理用户实现源文件
 * @details 该文件用于存放各类CAN中断Callback函数.
 *
 * @note    1. 所有弱函数默认均为空实现（位于 CAN_Cus.c），若用户在此文件中
 *              提供同名非弱函数，则链接器会自动采用用户版本。
 *          2. 建议将中断相关的业务逻辑集中于此文件，保持驱动核心代码的整洁。
 *          3. 若需要启用 CAN 中断，请务必在应用代码中调用 Cus_CAN_Device_t 的
 *              EnableInterrupt 函数，该函数会自动调用 Cus_CAN_NVIC_Config。
 *
 * @author  [R6Bandito]
 * @date    2025-03-30
 */


#include "Cus_CAN_IT.h"

/* ------------------------------------------------------------------- */
extern void Cus_CAN_RingRecvIT( Cus_CAN_Device_t *pDev, uint32_t FIFO );
extern void Cus_CAN_FeedCanTP(Cus_CAN_Device_t *pDev);

extern volatile uint8_t g_resend_done;
extern volatile uint8_t g_data_pending;
extern volatile uint8_t g_frameware_send;
/* ------------------------------------------------------------------- */



/* *************************************************************** */

void USB_LP_CAN1_RX0_IRQHandler( void )
{
  HAL_CAN_IRQHandler(Cus_CAN_getHandle(CAN1));
}


void CAN1_RX1_IRQHandler( void )
{
  HAL_CAN_IRQHandler(Cus_CAN_getHandle(CAN1));
}


void USB_HP_CAN1_TX_IRQHandler( void )
{
  HAL_CAN_IRQHandler(Cus_CAN_getHandle(CAN1));
}


void CAN1_SCE_IRQHandler( void )
{
  HAL_CAN_IRQHandler(Cus_CAN_getHandle(CAN1));
}


void HAL_CAN_RxFifo0MsgPendingCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    /* Don't Change this filed code!*/
    {
      Cus_CAN_RingRecvIT( Cus_CAN_getControlBlock(hcan->Instance), CAN_RX_FIFO0 );
      #ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
        Cus_CAN_Device_t * pDev = Cus_CAN_getControlBlock(hcan->Instance);
        CAN_RxHeaderTypeDef RxHeader;
        U8 buffer[8];
        pDev->Receive_IT(pDev, &RxHeader, buffer);
        Cus_Cantp_RecieveFrame(buffer, RxHeader.DLC, RxHeader.StdId);
      #endif 
    }
    /* Your Code Here. */
    {
      // .....
    }
  }

  #if defined(CAN2)
    if ( hcan->Instance == CAN2 )
    {
      {
        /* Don't Change this filed code!*/
        Cus_CAN_RingRecvIT( Cus_CAN_getControlBlock(hcan->Instance), CAN_RX_FIFO0 );
      }
      /* Your Code Here. */
      {
        // .....
      }
    }
  #endif // CAN2
}


void HAL_CAN_RxFifo1MsgPendingCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    /* Don't Change this filed code!*/
    {
      Cus_CAN_RingRecvIT( Cus_CAN_getControlBlock(hcan->Instance), CAN_RX_FIFO1 );
      #ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
        Cus_CAN_Device_t * pDev = Cus_CAN_getControlBlock(hcan->Instance);
        CAN_RxHeaderTypeDef RxHeader;
        U8 buffer[8];
        pDev->Receive_IT(pDev, &RxHeader, buffer);
        Cus_Cantp_RecieveFrame(buffer, RxHeader.DLC, RxHeader.StdId);
      #endif 
    }
    /* Your Code Here. */
    {
      // .....
    }
  }

  #if defined(CAN2)
    if ( hcan->Instance == CAN2 )
    {
      {
        /* Don't Change this filed code!*/
        Cus_CAN_RingRecvIT( Cus_CAN_getControlBlock(hcan->Instance), CAN_RX_FIFO0 );
      }
      /* Your Code Here. */
      {
        // .....
      }
    }
  #endif // CAN2
}


void HAL_CAN_RxFifo0FullCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_RxFifo1FullCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_TxMailbox0CompleteCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    {
      #ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
        Cus_Cantp_TxConfirmation((U8 *)hcan->Instance, 1);
      #endif

      #if (USE_SEND_ASYNC)
        Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(hcan->Instance);
        if ( pDev )
        {
          Cus_CAN_ProcessTxQueue(pDev);
        }
      #endif 
    }
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_TxMailbox1CompleteCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    {
      #ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
        Cus_Cantp_TxConfirmation((U8 *)hcan->Instance, 2);
      #endif

      #if (USE_SEND_ASYNC)
        Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(hcan->Instance);
        if ( pDev )
        {
          Cus_CAN_ProcessTxQueue(pDev);
        }
      #endif 
    }
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_TxMailbox2CompleteCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    {
      #ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
        Cus_Cantp_TxConfirmation((U8 *)hcan->Instance, 4);
      #endif

      #if (USE_SEND_ASYNC)
        Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(hcan->Instance);
        if ( pDev )
        {
          Cus_CAN_ProcessTxQueue(pDev);
        }
      #endif       
    }
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_TxMailbox0AbortCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_TxMailbox1AbortCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_TxMailbox2AbortCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_WakeUpFromRxMsgCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_ErrorCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}


void HAL_CAN_SleepCallback( CAN_HandleTypeDef *hcan )
{
  if ( hcan->Instance == CAN1 )
  {
    // Your Code Here.
    {

    }
  }
}
/* *************************************************************** */


/*
  Default Fault Hook Defines.
  错误处理相关Hook函数默认定义.
*/
/* ------------------------------------------------------------------------- */
__weak void Cus_FilterConfigFailed_Hook( CAN_HandleTypeDef *hcan, const CANFilterConfig_t *pFilterConf, HAL_StatusTypeDef hal_status )
{
  UNUSED(hcan);
  UNUSED(pFilterConf);
  UNUSED(hal_status);
}


__weak void Cus_CANInitFailed_Hook( CAN_HandleTypeDef *hcan, const CANInitConfig_t *pInitConf, HAL_StatusTypeDef hal_status )
{
  UNUSED(hcan);
  UNUSED(pInitConf);
  UNUSED(hal_status);
}


__weak void Cus_CANStartFailed_Hook( CAN_HandleTypeDef *hcan, HAL_StatusTypeDef hal_status )
{
  UNUSED(hcan);
  UNUSED(hal_status);
}


__weak void Cus_CANSendFailed_Hook( Cus_CAN_Device_t *pDev, HAL_StatusTypeDef hal_status )
{
  UNUSED(pDev);
  UNUSED(hal_status);
}


__weak void Cus_CANRecvITFull_Hook( Cus_CAN_Device_t *pDev )
{
  UNUSED(pDev);
}


__weak void Cus_CANRecvITFailed_Hook ( Cus_CAN_Device_t *pDev )
{
  UNUSED(pDev);
}











