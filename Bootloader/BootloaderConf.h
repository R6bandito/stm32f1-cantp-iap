#ifndef __BOOTLOADER_CONF_H__
#define __BOOTLOADER_CONF_H__


/* ****************************** */
  #include "stm32f1xx_hal.h"            // 按需更换所使用头文件.
/* ****************************** */


/* -------------- Feature ------------------- */
  #define RELEASE                 (1)

  #define USE_UTILS_DEBUG         (1)
  #define USE_UTILS_SYSCONF       (1)
  #define USE_RECOVERY_APP        (0)
  #define USE_POWER_FAIL_RESUME   (1)         // 是否启用断电续传. 0=不启用,升级过程中断电后下次启动重新进行擦写流程. 1=启用,升级过程断电后，下次启动继续烧写.
  #define USE_IWDG                (0)         // 接管IWDG喂狗操作. 0=未开启IWDG，耗时操作不处理喂狗. 1=开启IWDG，耗时操作自动处理喂狗. 
/* ------------------------------------------ */

/* -------------- DB Config ------------------- */
#if (USE_UTILS_DEBUG)
  #define DB_UART_INSTANCE      (USART1)
  #define DB_UART_GPIOPORT      (GPIOA)
  #define DB_UART_RX_PIN        (GPIO_PIN_10)
  #define DB_UART_TX_PIN        (GPIO_PIN_9)
  #define DB_UART_BAUDRATE      (115200UL)
#endif
/* ------------------------------------------ */

/* -------------- POWER_FAIL_RESUME Config ------------------- */
#if (USE_POWER_FAIL_RESUME)
  #define PWRFAIL_CONF_IGNORE_ERROR   (1)
#endif 
/* ----------------------------------------------------------- */


/* -------------- Core Config ------------------- */
// @brief Bootloader memory layout configuration.
// @note  Please adjust these addresses and sizes according to your actual MCU flash partitioning.
//        The layout typically consists of: Bootloader | Application | Download (firmware staging) | IAP Info.
//
// 1. BOOTLOADER_START_ADDRESS / SIZE:   Region that holds the bootloader code itself.
//                                       Must match the linker script.
//
// 2. APP_START_ADDRESS / APP_REGION_SIZE:   Region for the main application firmware.
//                                            Bootloader will jump here after update or timeout.
//
// 3. DOWNLOAD_START_ADDRESS / DOWNLOAD_REGION_SIZE:   Temporary storage for incoming firmware image.
//                                                     Bootloader writes received data here before verifying and copying.
//
// 4. IAP_INFO_STRUCT_START_ADDR / IAP_INFO_STRUCT_REGION_SIZE:   Persistent structure storing update status,
//                                                                version, magic word, etc.
//       It is strongly recommended to place this structure in the last page or the last sector of the flash,
//       so that it is not accidentally overwritten by application or download region expansion.
//
// 5. IAP_MAGIC_WORD:   A known pattern written at the beginning of the IAP info structure to indicate
//                      valid update information (e.g., 0xAA55).
//
// Example for STM32F103ZET6 with 512KB total flash:
//   Bootloader:   0x08000000, 32KB
//   App:          0x08008000, 224KB
//   Download:     0x08040000, 224KB
//   IAP Info:     0x0807F800,  2KB  (last 2KB of 512KB)
//   Total: 32+224+224+2 = 482KB (fit in 512KB, leaving some free space)
/* -------------- Core Config & Define ------------------- */
  #define BOOTLOADER_START_ADDRESS          (0x08000000UL)
  #define BOOTLOADER_SIZE                   (0x00008000UL)    // 32KB

  #define APP_START_ADDRESS                 (0x08008000UL)
  #define APP_REGION_SIZE                   (0x00038000UL)    // 224KB

  #define DOWNLOAD_START_ADDRESS            (0x08040000UL)
  #define DOWNLOAD_REGION_SIZE              (0x00038000UL)    // 224KB
  /* 32 + 224 + 224 = 480 KB. */

  #define IAP_INFO_STRUCT_START_ADDR        (0x0807F800UL)
  #define IAP_INFO_STRUCT_REGION_SIZE       (0x00000800UL)    // 2KB

  #define MCU_SRAM_BASE_ADDR                (SRAM_BASE) 
  #define MCU_SRAM_SIZE                     (64 * 1024)

  #define IAP_MAGIC_WORD                    (0xAA55UL)  
  #define SIZE_PER_PAGE_KB                  (2)              // 2KB Per Page.


  #if (USE_RECOVERY_APP)
    #define RECOVERY_APP_START_ADDR         (0x0807C000UL)
    #define RECOVERY_APP_REGION_SIZE        (0x00004000UL)   // 16kb
    #define MAX_FAILED_COUNT                (4)              // 连续重启次数阈值

    #define RECOVERY_BKP_ADDR               (BKP_BASE)
    #define RECOVERY_BKP_MAGIC              (0xDEADUL)   // 验证魔数
  #endif // USE_RECOVERY_APP

  #if (USE_POWER_FAIL_RESUME)  
    #define RESUME_BKP_MAGIC_ADDR           (BKP_BASE + 0x0C)   // 偏移两个BKP寄存器(前两个寄存器用于USE_RECOVERY_APP功能).
    #define RESUME_BKP_STATE_ADDR           (BKP_BASE + 0x10)   
    #define RESUME_BKP_PAGE_ADDR            (BKP_BASE + 0x14)
    #define RESUME_BKP_ERROR_FLAG_ADDR      (BKP_BASE + 0x18) // 错误标志.0位有效.

    #define PWRFAIL_RESUME_BKP_MAGIC        (0xBABAUL)   // 验证魔数.
  #endif // USE_POWER_FAIL_RESUME

/* ------------------------------------------ */



/* -------------- Error Define ------------------- */
  #define IAP_ERRCODE_INVALID_STACKTOPADDR  (0x01UL)
  #define IAP_ERRCODE_INVALID_RESETHANDLER  (0x01UL << 1)



/* ------------------------------------------ */

#endif 
