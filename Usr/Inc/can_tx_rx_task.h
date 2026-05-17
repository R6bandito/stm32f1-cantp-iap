#ifndef __CAN_TX_RX_TASK_H__
#define __CAN_TX_RX_TASK_H__


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Cus_CANTP.h"


/* ***************************************** */
  #define SENSOR_QUEUE_LENGTH              (8)      // 4 帧 ✖ 2 = 8.
  #define HARDWARE_RECV_BUF_SIZE           (1280)   // 1.25kb.

  #define FW_FRAME_TYPE_INFO               (0xF0)   // 固件信息帧 [0xF0 | size:4 | type:1]
  #define FW_FRAME_TYPE_END                (0xFE)
  #define FW_PACK_HEADER_INFO              (0xF1)          // 固件信息包头
  #define FW_PACK_HEADER_DATA              (0xF2)          // 固件数据包头
  #define FW_PACK_HEADER_END               (0xFE)          // 固件发送结束包头

  #define DOWNLOAD__ADDRESS                (0x08040000UL)
  #define DOWNLOAD_FIELD_SIZE              (0x00038000UL)    // 224KB
  #define FLASH_SIZE_PER_PAGE_BYTES        (2048)
/* ***************************************** */


/* 传感器数据队列元素类型 */
typedef struct 
{
  uint8_t data_type;        // C8T6 侧发送的数据类型.( 例：SENSOR_TYPE_RPM, SENSOR_TYPE_HUM_TEMP等 )
  uint16_t data_rpm;
  uint8_t data_spd;
  uint8_t data_humidity;
  uint8_t data_temp;
  uint8_t data_coolant;

} SensorFrame_t;


void ZET6_CantpDataIndication( Cus_CANTp_Conn_t *pConn, const U8 *data, U32 len );

void cTask_cantpMainFunction( void *pvParameter );
void cTask_cantpRxFunction( void *pvParameter );

#endif // __CAN_TX_RX_TASK_H__
