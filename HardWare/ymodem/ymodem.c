/**
  ******************************************************************************
  * @file    STM32F2xx_IAP/src/ymodem.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    02-May-2011
  * @brief   This file provides all the software functions related to the ymodem 
  *          protocol.
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

/** @addtogroup STM32F2xx_IAP
  * @{
  */ 
  
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "ymodem.h"
#include "string.h"
#include "crc32table.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t FileName[FILE_NAME_LENGTH];

UART_HandleTypeDef huart2;

uint8_t *g_bridge_data;
uint32_t g_bridge_len;
uint8_t fw_data_buf[1025];
static uint32_t s_CRC_C8T6;
uint32_t g_Hardware_size;

extern volatile bool is_ACKRecev;
extern volatile bool is_NAKRecv;

extern uint8_t C8T6_SendAndWaitACK( const uint8_t *data, uint32_t len, uint32_t timeout );
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


static void CRC32_Update( uint32_t *pCRC, const uint8_t *pData, uint32_t data_len )
{
  const uint32_t *table = crc32_table;
  for (uint32_t i = 0; i < data_len; i++)
  {
      *pCRC = (*pCRC >> 8) ^ table[(*pCRC ^ pData[i]) & 0xFF];
  }
}


static void CRC32_Init( uint32_t *pCRC )
{
  if ( !pCRC )    return;

  *pCRC = 0xFFFFFFFF;
}

void Ymodem_UARTInit( void )
{ 
  // port_index: GPIOA=0, GPIOB=1, GPIOC=2 ...
  uint32_t port_index = ((uint32_t)YMODEM_UART_PORT - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE);
  RCC->APB2ENR |= (0x01UL << (2 + port_index));     // 开Ymodem GPIO时钟.
  __nop();

  // 开 Ymodem 串口时钟.
  if ( YMODEM_UART_INSTANCE == USART1 )
  {
      RCC->APB2ENR |= RCC_APB2ENR_USART1EN;   // bit 14
  }
  else if ( YMODEM_UART_INSTANCE == USART2 )
  {
      RCC->APB1ENR |= RCC_APB1ENR_USART2EN;   // bit 17
  }
  else if ( YMODEM_UART_INSTANCE == USART3 )
  {
      RCC->APB1ENR |= RCC_APB1ENR_USART3EN;   // bit 18
  }
  // UART4/UART5 在 F1 上不常用，如有需要请自行添加.
  __NOP();
  
  GPIO_InitTypeDef YmodemInitStructure;

  YmodemInitStructure.Mode = GPIO_MODE_AF_PP;
  YmodemInitStructure.Pin = YMODEM_UART_TX_PIN;
  YmodemInitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(YMODEM_UART_PORT, &YmodemInitStructure);

  YmodemInitStructure.Mode = GPIO_MODE_AF_INPUT;
  YmodemInitStructure.Pin = YMODEM_UART_RX_PIN;
  YmodemInitStructure.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(YMODEM_UART_PORT, &YmodemInitStructure);

  huart2.Instance = YMODEM_UART_INSTANCE;
  huart2.Init.BaudRate = YMODEM_UART_BAUDRATE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;

  if ( HAL_UART_Init(&huart2) != HAL_OK )
  {
    for( ; ; );
  }
}

/**
  * @brief  Receive byte from sender
  * @param  c: Character
  * @param  timeout: Timeout
  * @retval 0: Byte received
  *        -1: Timeout
  */
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
    if (SerialKeyPressed(c) == 1)
    {
      return 0;
    }
  }
  return -1;
}

/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
static uint32_t Send_Byte (uint8_t c)
{
  SerialPutChar(c);
  return 0;
}

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    -1: abort by sender
  *    >0: packet length
  * @retval 0: normally return
  *        -1: timeout or packet error
  *         1: abort by user
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t i, packet_size;
  uint8_t c;
  *length = 0;
  if (Receive_Byte(&c, timeout) != 0)
  {
    return -1;
  }
  switch (c)
  {
    case SOH:
      packet_size = PACKET_SIZE;
      break;
    case STX:
      packet_size = PACKET_1K_SIZE;
      break;
    case EOT:
      return 0;
    case CA:
      if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
      {
        *length = -1;
        return 0;
      }
      else
      {
        return -1;
      }
    case ABORT1:
    case ABORT2:
      return 1;
    default:
      return -1;
  }
  *data = c;
  for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
  {
    if (Receive_Byte(data + i, timeout) != 0)
    {
      return -1;
    }
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))//序号和补码
  {
    return -1;
  }
  *length = packet_size;
  return 0;
}

/**
  * @brief  Receive a file using the ymodem protocol.
  * @param  buf: Address of the first byte.
  * @retval The size of the file.
  */
 int32_t Ymodem_Receive ()
 {
   uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD]; 
   uint8_t file_size[FILE_SIZE_LENGTH]; 
   uint8_t *file_ptr; 
   int32_t i, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
   static uint8_t s_req_sendCount = 0;

   s_req_sendCount = 0;
 
   CRC32_Init(&s_CRC_C8T6);
 
   for (session_done = 0, errors = 0, session_begin = 0; ;)
   {
     for (packets_received = 0, file_done = 0; ;)
     {
       switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
       {
         case 0:
           errors = 0;
           switch (packet_length)
           {
             /* Abort by sender */
             case - 1:
               Send_Byte(ACK);
               return 0;
             /* End of transmission */
             case 0:
               /* 构建 END 包.  */
               uint8_t end[6] = { 0 };
               end[0] = FW_PACK_HEADER_END;
               uint32_t crc_c8t6_finalize = s_CRC_C8T6 ^ 0xFFFFFFFF;
               *(uint32_t *)(end + 1) = crc_c8t6_finalize;

               /* 发送 END 包 */
               uint8_t eot_ack = C8T6_SendAndWaitACK(end, sizeof(end), 150);
               if ( eot_ack )
               {
                 /* ACK 接收成功. */
                 Send_Byte(ACK);
                 file_done = 1;
               }
               else 
               {
                /* 请求上层重发. */
                Send_Byte(NAK);
               }

               /* 保证下一次传输不受上次计数残留影响 */
               is_ACKRecev = false;
               is_NAKRecv = false;
               break;
             /* Normal packet */
             default:
               if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
               {
                 Send_Byte(NAK);
               }
               else
               {
                 if (packets_received == 0)
                 {
                   /* Filename packet */
                   if (packet_data[PACKET_HEADER] != 0)
                   {
                     /* Filename packet has valid data */
                     for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                     {
                       FileName[i++] = *file_ptr++;
                     }
                     FileName[i++] = '\0';
                     for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                     {
                       file_size[i++] = *file_ptr++;
                     }
                     file_size[i++] = '\0';
                     Str2Int(file_size, &size);
 
                     {
                       /* 保存固件大小信息 为后续留用 */
                       g_Hardware_size = (uint32_t)size;
 
                       /* 构建固件info信息 */
                       uint8_t info[6] = { 0 };
                       info[0] = FW_PACK_HEADER_INFO;
                       *(uint32_t *)(info + 1) = g_Hardware_size;
                       info[5] = (packet_length == PACKET_1K_SIZE) ? 0 : 1;
 
                       /* 发送info包，等待ACK */
                       uint8_t hReturn = C8T6_SendAndWaitACK(info, sizeof(info), 100);
                       if ( !hReturn )
                       {
                         /* 发送失败.(请求提交失败/等待ACK失败) 结束本次通信 */
                         printf("C8T6_SendAndWaitACK Timeout. in Ymodem_Receive.\r\n");
                         Send_Byte(CA);
                         Send_Byte(CA);

                         return 0;
                       }
                     }

                     /* 消费掉 ACK 标志.(C8T6_SendAndWaitACK内部只检测ACK，而不消费ACK. 关于ACK复位由上层确保) */
                     is_ACKRecev = false;
                     
                     Send_Byte(ACK);
                     Send_Byte(CRC16);
                   }
                   /* Filename packet is empty, end session */
                   else
                   {
                     Send_Byte(ACK);
                     file_done = 1;
                     session_done = 1;
                     break;
                   }
                 }
                 /* Data packet */
                 else
                 {
                   g_bridge_data = packet_data + PACKET_HEADER;
                   g_bridge_len = packet_length;
 
                   /* 每收到一包数据，校验CRC32 */
                   CRC32_Update(&s_CRC_C8T6, g_bridge_data, g_bridge_len);
 
                   {
                     /* 发送固件数据包(带包头) */
                     fw_data_buf[0] = FW_PACK_HEADER_DATA;
                     memcpy(&fw_data_buf[1], g_bridge_data, g_bridge_len);
 
                     /* 发送包数据(多帧发送耗时较长，适当延长Timeout) */
                     uint8_t hReturn = C8T6_SendAndWaitACK(fw_data_buf, g_bridge_len + 1, 250);
                     if ( !hReturn )
                     {
                      s_req_sendCount++;
                      if ( s_req_sendCount > 3 )
                      {
                        printf("C8T6_SendAndWaitACK Timeout in 3 times. Connection close.\r\n");
                        Send_Byte(CA);
                        Send_Byte(CA);

                        return 0;
                      }

                      /* 出错. 请求发送方重发该包. */
                      printf("C8T6_SendAndWaitACK Timeout in data packet.\r\n");
                      is_ACKRecev = false;
                      Send_Byte(NAK);
                      continue;
                     }
                     s_req_sendCount = 0;
                     is_ACKRecev = false;
                   }
 
                   Send_Byte(ACK);
                 }
                 packets_received ++;
                 session_begin = 1;
               }
           }
           break;
         case 1:
           Send_Byte(CA);
           Send_Byte(CA);
           return -3;
         default:
           if (session_begin > 0)
           {
             errors ++;
           }
           
           if (errors > MAX_ERRORS)
           {
             Send_Byte(CA);
             Send_Byte(CA);
             return 0;
           }
 
           Send_Byte(CRC16);
           break;
       }
       if (file_done != 0)
       {
         break;
       }
     }
     if (session_done != 0)
     {
       break;
     }
   }
   return (int32_t)size;
 }

/**
  * @brief  check response using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
  * @brief  Prepare the first block
  * @param  timeout
  *     0: end of transmission
  * @retval None
  */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  
  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
  * @brief  Prepare the data packet
  * @param  timeout
  *     0: end of transmission
  * @retval None
  */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/**
  * @brief  Update CRC16 for input byte
  * @param  CRC input value 
  * @param  input byte
  * @retval None
  */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
  uint32_t crc = crcIn;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  }
  
  while(!(in & 0x10000));

  return crc & 0xffffu;
}


/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd)
    crc = UpdateCRC16(crc, *data++);
 
  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd )
    sum += *data++;

  return (sum & 0xffu);
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
  * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t filename[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum;
  uint16_t tempCRC;
  uint16_t blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors, ackReceived, size = 0, pktSize;

  errors = 0;
  ackReceived = 0;
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    filename[i] = sendFileName[i];
  }
  CRC16_F = 1;
    
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

    /* Send CRC or Check Sum based on CRC16_F */
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCRC >> 8);
       Send_Byte(tempCRC & 0xFF);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCheckSum);
    }
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transferred correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
  
  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
  
  
  /* Resend packet if NAK  for a count of 10 else end of communication */
  while (size)
  {
    /* Prepare next packet */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* Send next packet */
      if (size >= PACKET_1K_SIZE)
      {
        pktSize = PACKET_1K_SIZE;
       
      }
      else
      {
        pktSize = PACKET_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* Send CRC or Check Sum based on CRC16_F */
      /* Send CRC or Check Sum based on CRC16_F */
      if (CRC16_F)
      {
         tempCRC = Cal_CRC16(&packet_data[3], pktSize);
         Send_Byte(tempCRC >> 8);
         Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      
      /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
      {
        ackReceived = 1;  
        if (size > pktSize)
        {
           buf_ptr += pktSize;  
           size -= pktSize;
           if (blkNumber == (CON_DEVICE_RECV_BUF/1024))
           {
             return 0xFF; /*  error */
           }
           else
           {
              blkNumber++;
           }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* Resend packet if NAK  for a count of 10 else end of communication */
    
    if (errors >=  0x0A)
    {
      return errors;
    }
    
  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
    if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
    {
      ackReceived = 1;  
    }
    else
    {
      errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  
  /* Last packet preparation */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

    /* Send CRC or Check Sum based on CRC16_F */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transferred correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));

  /* Resend packet if NAK  for a count of 10  else end of communication */
  if (errors >=  0x0A)
  {
    return errors;
  }  
  
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
    if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
    {
      ackReceived = 1;  
    }
    else
    {
      errors++;
    }
  }while (!ackReceived && (errors < 0x0A));

  if (errors >=  0x0A)
  {
    return errors;
  }
  return 0; /* file transmitted successfully */
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
