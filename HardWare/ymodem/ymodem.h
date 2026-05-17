/**
  ******************************************************************************
  * @file    STM32F2xx_IAP/inc/ymodem.h 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    02-May-2011
  * @brief   This file provides all the software function headers of the ymodem.c 
  *          file.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __YMODEM_H_
#define __YMODEM_H_

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (256)
#define FILE_SIZE_LENGTH        (16)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             (0x800000)
#define MAX_ERRORS              (5)

#define YMODEM_UART_INSTANCE    (USART2)
#define YMODEM_UART_BAUDRATE    (115200)

#define YMODEM_UART_PORT        (GPIOA)
#define YMODEM_UART_TX_PIN      (GPIO_PIN_2)
#define YMODEM_UART_RX_PIN      (GPIO_PIN_3)

#define FW_PACK_HEADER_INFO     (0xF1)          // 固件信息包头
#define FW_PACK_HEADER_DATA     (0xF2)          // 固件数据包头
#define FW_PACK_HEADER_END      (0xFE)          // 固件发送结束包头

#define CON_DEVICE_RECV_BUF     (224 * 1024)    // 对端通信设备最大接收缓冲区.

/* Exported functions ------------------------------------------------------- */
int32_t Ymodem_Receive ();
uint8_t Ymodem_Transmit (uint8_t *,const  uint8_t* , uint32_t );
void Ymodem_UARTInit( void );

#endif  /* __YMODEM_H_ */


/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
