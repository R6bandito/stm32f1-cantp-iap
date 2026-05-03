/**
  ******************************************************************************
  * @file    Templates/Src/stm32f1xx.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "c8t6_main.h"
#include "stm32f1xx_it.h"
#include "sensor_param_sim.h"
#include <string.h>
   

/* *********** main全局数据 ************** */
  extern TIM_HandleTypeDef htim4;
  extern SensorData_t g_sensor_data;
  extern volatile bool dht11_sample_flag;
  extern volatile bool data_send_sample_flag;
  extern uint8_t iap_commandBuff[8];
  extern volatile bool iap_start_flag;
/* ************************************** */

/* ********* Ymodem 全局数据. ************ */
  extern uint8_t *g_bridge_data;
  extern uint32_t g_bridge_len;
  extern volatile uint8_t g_resend_done;
  extern volatile uint8_t g_data_pending;
  extern UART_HandleTypeDef huart2;
/* ************************************** */

volatile uint8_t it_count;
volatile uint8_t it_count_sendData;

/* --------------------------------------- */
void TIM4_IRQHandler( void )
{
  HAL_TIM_IRQHandler(&htim4);
}


void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef *htim )
{
  if ( htim->Instance == TIM4 )
  {
    C8T6_Sensor_EngineRPMSimUpdate();
    C8T6_Sensor_VehicleSpeedSimUpdate();
    C8T6_Sensor_CoolantSimUpdate();

    it_count++;
    it_count_sendData++;

    // 每 2s 读取一次DHT11.
    if ( it_count >= 20 && dht11_sample_flag != true )
    {
      dht11_sample_flag = true;
      it_count = 0;
    }

    // 每 500ms 向对端通信设备发送一次CAN报文数据.
    if ( it_count_sendData >= 5 && data_send_sample_flag != true )
    {
      data_send_sample_flag = true;
      it_count_sendData = 0;
    }
  }
}


void PendSV_Handler(void)
{
  if ( g_data_pending )
  {
    // 存在数据挂起. 待转发.
    g_data_pending = 0;

    // 发起一次CANTP通信请求. 转发该包数据.
    // __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);
    // uint8_t hReturn = Cus_Cantp_Transmit(0, 0, g_bridge_data, g_bridge_len, CAN1);
    // __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
    // if ( hReturn == 0 )
    // {
    //   // 发送失败.不重置标志. 等待发送超时.
    //   return;
    // }

  }
}


void USART2_IRQHandler( void )
{
  HAL_UART_IRQHandler(&huart2);
}


void HAL_UART_RxCpltCallback( UART_HandleTypeDef *huart )
{
  if ( huart2.Instance == USART2 )
  {
    if ( memcmp(iap_commandBuff, "UPDATE", 6) == 0 && iap_start_flag != true )  // 判断升级命令.
    {
      iap_start_flag = true;  // 接收到升级命令. 置标志.
      memset(iap_commandBuff, 0, sizeof(iap_commandBuff));  
    }
  }
}
/* --------------------------------------- */


/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles SVCall exception.
* @param  None
* @retval None
*/
void SVC_Handler(void)
{

}



void SysTick_Handler(void)
{
	HAL_IncTick();
}



/**
* @brief  This function handles SysTick Handler.
* @param  None
* @retval None
*/


/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}





/******************************************************************************/
/*                 STM32F1xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f1xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/


/**
  * @}
  */ 

/**
  * @}
  */
