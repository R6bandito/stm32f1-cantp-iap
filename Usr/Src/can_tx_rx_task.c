#include "can_tx_rx_task.h"
#include "Cus_CANTP.h"
#include "sensor_data.h"
#include "Cus_Flash.h"
#include "crc32table.h"


/* **************************************************** */
  xQueueHandle g_sensorQueue_handle = NULL;
  extern 	Cus_CANTp_Conn_t *pConn_Tx1;
  extern TaskHandle_t Cantp_RxLogic_Taskhandle;
  extern TaskHandle_t Iap_Update_Taskhandle;

  BaseType_t is_HardwareRecv = pdFALSE;                      // 固件长报文接收标志.
  extern uint8_t Cantp_RecvBuffer[HARDWARE_RECV_BUF_SIZE];   // 固件接收数据包缓冲区.
  uint32_t g_HardwarePackageLen;                             // 该包有效数据长度.
  uint16_t g_PacketsInCurrentPage;                           //  当前FLASH页内包计数.
  uint8_t g_YmodemTransferType;                              // 0=1024 Bytes(Per Package). 1=128 Bytes.
  uint8_t g_FirmwareTransferComplete;                        // 固件传输完成标志位.
  uint8_t g_isACKcouldBeSend;
  uint32_t g_FirmwareTotalSize;                               // 传输总大小.

  uint32_t g_ZET6_CRC32Val;                                  // ZET6 侧累积CRC校验码.
  uint32_t g_C8T6_CRC32Val;                                  // C8T6 侧累积CRC校验码.

  uint8_t iap_RequestCheck;                                   // IAP标志位.
  uint8_t g_NAK_Pending;                                      // CRC校验失败. NAK挂起.
/* **************************************************** */

/* **************************************************** */
  void cTask_cantpMainFunction( void *pvParameter );
  void cTask_cantpRxFunction( void *pvParameter );

  void ZET6_CantpDataIndication( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len );
  static void CRC32_Init( uint32_t *pCRC );
  static void SensorQueue_Initial( void );
  static void CRC32_Update(uint32_t *pCRC, const uint8_t *pData, uint32_t data_len);
/* **************************************************** */


void cTask_cantpMainFunction( void *pvParameter )
{
  /* 记录上一次被唤醒的时刻 */
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while(1)
  {
    Cus_Cantp_MainFunction();

    vTaskDelayUntil(xLastWakeTime, pdMS_TO_TICKS(1));
  }
}


void cTask_cantpRxFunction( void *pvParameter )
{
  SensorFrame_t frame;
  Cus_Flash_Page_t *pPage = NULL;
  uint16_t pageFullCount = 0;         // 填充满一页需要的包数目.
  uint32_t currentPageAddr = DOWNLOAD__ADDRESS;

  SensorQueue_Initial();
  Cus_Flash_State_t ret = Factory_GetPageControlBlock(&pPage);
  if ( ret != CUS_FLASH_OK )    for( ; ; );

  uint8_t *offset_pageBuffer = pPage->PageDataBuffer;   // pPage页缓冲区写入偏移.

  while(1)
  {
    xQueueReceive(g_sensorQueue_handle, &frame, pdMS_TO_TICKS(10));    // frame 后续通过LVGL TASK提供的API给到显示层.

    taskENTER_CRITICAL();
    /* 检查长报文标志. */
    if ( is_HardwareRecv && g_HardwarePackageLen )
    {      
      if ( g_YmodemTransferType == 0 )
      {
        pageFullCount = 2;
      }
      else if ( g_YmodemTransferType == 1 )
      {
        pageFullCount = 16;
      }

      /* 将固件写入缓冲区. */
      memcpy(offset_pageBuffer, Cantp_RecvBuffer, g_HardwarePackageLen);
      CRC32_Update(&g_ZET6_CRC32Val, Cantp_RecvBuffer, g_HardwarePackageLen);    // CRC32校验计算.
      g_PacketsInCurrentPage++;
      offset_pageBuffer += g_HardwarePackageLen;            // 写入地址偏移.

      /* 可以发送 ACK. (每包数据均回复ACK)*/
      g_isACKcouldBeSend = 1;

      /* 消费完一次数据后，立马清除标志 */
      is_HardwareRecv = pdFALSE;
      g_HardwarePackageLen = 0;
      taskEXIT_CRITICAL();

      taskENTER_CRITICAL();
      if ( g_PacketsInCurrentPage == pageFullCount )
      {
        /* 页填充完毕 开始写入. */
        pPage->PageAddress = currentPageAddr;
        taskEXIT_CRITICAL();

        /* 再写入(Cus_Flash_WritePage内部会自动擦除) */
        Cus_Flash_WritePage(pPage);                // 该API内部自带临界保护. 为了防止提前解锁导致FreeRTOS临界区受损. 在进入该API前保证RTOS临界退出.

        taskENTER_CRITICAL();
        /* 更新FLASH到下一页. 准备下一包接收 */
        currentPageAddr += FLASH_SIZE_PER_PAGE_BYTES;
        offset_pageBuffer = pPage->PageDataBuffer;
        pPage->Reset(pPage);

        /* FLASH写入完毕. 重置状态，并准备发送ACK.*/
        g_PacketsInCurrentPage = 0;
      }
    }
    taskEXIT_CRITICAL();

    taskENTER_CRITICAL();
    if ( g_FirmwareTransferComplete && g_PacketsInCurrentPage > 0 )
    {
      /* 还有未满一页的残余数据，强制刷写. */
      pPage->PageAddress = currentPageAddr;

      taskEXIT_CRITICAL();
      Cus_Flash_WritePage(pPage);

      taskENTER_CRITICAL();
      g_PacketsInCurrentPage = 0;
      currentPageAddr = DOWNLOAD__ADDRESS;
    }
    else if ( g_FirmwareTransferComplete && g_PacketsInCurrentPage == 0 )
    {
      /* 没有残余数据. 恢复变量状态 */
      currentPageAddr = DOWNLOAD__ADDRESS;
    }

    if ( g_FirmwareTransferComplete )
    {
      /* CRC 校验. */
      uint32_t ZET6_CRC32Finalize = g_ZET6_CRC32Val ^ 0xFFFFFFFF;

      if ( ZET6_CRC32Finalize == g_C8T6_CRC32Val )
      {
        /* CRC校验成功.置IAP标志. 并将IAP任务优先级提升到最高 */
        g_ZET6_CRC32Val = ZET6_CRC32Finalize;     // 保留最终值供IAP任务使用.
        iap_RequestCheck = 1;
        g_isACKcouldBeSend = 1;
        vTaskPrioritySet(Iap_Update_Taskhandle, configMAX_PRIORITIES - 1);
        xTaskNotifyGive(Iap_Update_Taskhandle);
      }
      else 
      {
        /* CRC校验失败 */
        g_NAK_Pending = 1;
        g_isACKcouldBeSend = 0;
      }

      g_FirmwareTransferComplete = 0;
    }
    taskEXIT_CRITICAL();

    taskENTER_CRITICAL();
    if ( g_isACKcouldBeSend )
    {
      /* 发送ACK. */
      uint8_t ACK = 0xFE;
      S8 hReturn = Cus_Cantp_startTransmit(pConn_Tx1, &ACK, 1);
      if ( hReturn > 0 )
      {
        /* 发送请求成功. 清标志. */
        g_isACKcouldBeSend = 0;
      }
    }
    else if ( g_NAK_Pending )
    {
      uint8_t NAK = 0xFF;
      S8 hReturn = Cus_Cantp_startTransmit(pConn_Tx1, &NAK, 1);
      if ( hReturn > 0 )
      {
        g_NAK_Pending = 0;
      }
    }
    taskEXIT_CRITICAL();
  }
}


void ZET6_CantpDataIndication( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len )
{
  if ( !pConn || !data || len == 0 )    return;

  if ( len >= 2 && len < 4 )
  {
    /* 根据长度可以知道该数据为 C8T6 侧所发出的传感器数据单帧 */
    uint8_t frameType = data[0];
    uint16_t payload = 0;
    static uint8_t s_recv_mask = 0;           // 已收字段位图.
    static SensorFrame_t s_Queueframe; 

    /* 防丢帧卡死 容错机制 */
    /* C8T6侧逻辑中，RPM帧作为起始.此处通过判断RPM帧来断定一次有效通讯的起始.  */
    if ( data[0] == SENSOR_TYPE_RPM )
    {
      /* 新通讯开始. 无条件重置掩码. */
      s_recv_mask = 0;    
    }
    else if ( s_recv_mask == 0 )
    {
      /* 连RPM本帧都没收到. 直接返回放弃此次通讯. */
      return;
    }

    switch (frameType)
    {
      case SENSOR_TYPE_RPM:     
      {
        /* 提取高8位和低8位 */
        payload = (((payload | data[2]) << 8) | data[1]);
        s_Queueframe.data_rpm = payload;

        /* 更新位图 */
        s_recv_mask |= (1 << 0);  

        break;
      }

      case SENSOR_TYPE_SPD:   
      {
        s_Queueframe.data_spd = data[1];
        s_recv_mask |= (1 << 1);
        break;
      }  

      case SENSOR_TYPE_HUM_TEMP:
      {
        s_Queueframe.data_temp = data[1];
        s_recv_mask |= (1 << 2);

        s_Queueframe.data_humidity = data[2];
        s_recv_mask |= (1 << 3);

        break;
      }    

      case SENSOR_TYPE_COOLANT:
      {
        s_Queueframe.data_coolant = data[1];
        s_recv_mask |= (1 << 4);
        break;
      }     
    
      default:    return;
    }

    if ( s_recv_mask == 0x1F )
    {
      /*5 个字段到齐 发队列 */
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      if ( g_sensorQueue_handle )
      {
        xQueueSendFromISR(g_sensorQueue_handle, &s_Queueframe, &xHigherPriorityTaskWoken);
      }

      /* 清空位图. 准备下一轮接收 */
      s_recv_mask = 0;
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return;
  }

  /* 长报文接收逻辑. */
  switch (data[0])
  {
    case FW_PACK_HEADER_INFO:
    {
      /* 长报文信息帧(大小，类型) */
      if ( len < 6 )    return;
      g_YmodemTransferType = data[5];
      g_FirmwareTotalSize = *((uint32_t *)(data + 1));

      /* 置传输启动标志. */
      is_HardwareRecv = pdTRUE;        

      /* 开始新一轮的CRC累积 */
      CRC32_Init(&g_ZET6_CRC32Val);

      /* 置 ACK 发送标志位 */
      g_isACKcouldBeSend = 1;

      break;
    }
      
    case FW_PACK_HEADER_DATA:
    {
      uint16_t copy_size = 0;

      /* 准备接收固件数据包 */
      if ( g_YmodemTransferType == 0 ) 
      {
        copy_size = 1024;
      }
      else if ( g_YmodemTransferType == 1 )
      {
        copy_size = 128;
      }

      memcpy(Cantp_RecvBuffer, &data[1], copy_size);
      is_HardwareRecv = pdTRUE;
      g_HardwarePackageLen = copy_size;

      break;
    }

    case FW_PACK_HEADER_END:
    {
      /* 固件传输结束报文 */
      /* 收到结束帧. 结束本次通信. */
      g_FirmwareTransferComplete = 1;

      /* 接收C8T6则发来的CRC校验码. */
      if ( len >= 5 )
      {
        g_C8T6_CRC32Val = *(uint32_t *)(data + 1);
      }

      /* 关于此处通信结束后，全局相关变量的复原，在任务中进行 */
      break;
    }
    
    default:    break;
  }
}


static void SensorQueue_Initial( void )
{
  /* 初始化传感器数据接收队列 */
  g_sensorQueue_handle = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorFrame_t));
  if ( !g_sensorQueue_handle )    
  {
    /* 核心数据结构创建失败. 目前暂时先死循环处理 */
    for( ; ; );
  }
}


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


