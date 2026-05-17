#include "iap_task.h"
#include "Cus_Flash.h"


/* **************************************************** */
extern uint8_t iap_RequestCheck;
extern uint32_t g_FirmwareTotalSize;
extern uint32_t g_ZET6_CRC32Val;
extern uint8_t g_NAK_Pending;
extern uint8_t g_isACKcouldBeSend;


/* 注：该结构体第一份原型在 Bootloader.h 中. 由于ZET6工程不编译 Bootloader. 因此此处再次声明，注意同步问题！*/
typedef struct Bootloader_info
{
  uint16_t magic_word;
  uint16_t version;
  uint32_t app_size;
  uint32_t CRC32;

} IAP_Info_t;
/* **************************************************** */

/* ******************************************** */
  void cTask_iapUpdate( void * pvParameters );
/* ******************************************** */


void cTask_iapUpdate( void * pvParameters )
{
  static uint8_t verifyFailCount = 0;

  while(1)
  {
    /* 等待 Rx 任务发信号量 */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    /* 重置错误计数 */
    verifyFailCount = 0;

CHECK_POINT:
    /* IAP请求置起，新固件已被刷入DOWNLOAD区域. */
    if ( iap_RequestCheck )
    {
      IAP_Info_t iap_info = { 0 };

      iap_info.app_size = g_FirmwareTotalSize;
      iap_info.CRC32 = g_ZET6_CRC32Val;
      iap_info.magic_word = IAP_MAGIC;
      iap_info.version = 0x0001;

      /* 获取IAP所在页 页起始地址 */
      uint32_t pageStartaddr = Cus_Flash_GetPageStart(IAP_INFO_STRUCT_ADDR);

      /* 擦除IAP INFO所在页 */
      Cus_Flash_ErasePage(pageStartaddr);

      /* 写入 IAP_INFO 到指定位置 */
      Cus_Flash_WriteBuffer(IAP_INFO_STRUCT_ADDR, (uint8_t *)&iap_info, sizeof(iap_info));

      /* 写入验证 */
      bool is_WriteOK = Cus_Flash_VerifyBuffer(IAP_INFO_STRUCT_ADDR, (uint8_t *)&iap_info, sizeof(iap_info));
      if ( !is_WriteOK )
      {
        /* 写入验证失败. 先不允许发送ACK. 一段时间后重试 */
        verifyFailCount++;
        g_isACKcouldBeSend = 0;

        if ( verifyFailCount > 3 )
        {
          /* 多次写入出错. 放弃此次通讯.由Rx回复NAK */
          iap_RequestCheck = 0;
          g_NAK_Pending = 1;
          vTaskPrioritySet(NULL, 5);
          taskYIELD();
        }

        vTaskDelay(pdMS_TO_TICKS(5));   goto CHECK_POINT;
      }

      g_isACKcouldBeSend = 1;

      /* 放出时间片 等 Rx 中的ACK发完 */
      vTaskDelay(pdMS_TO_TICKS(100));

      /* 软复位. 接下来由Bootloader执行程序更新 */
      NVIC_SystemReset();
    }
  }
}





