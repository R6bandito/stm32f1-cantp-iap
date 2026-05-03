
/**
 * @file    Cus_Flash.h
 * @brief   STM32 内部 Flash 抽象驱动模块
 * 
 * @details 本模块通过设备宏（DEVICE_STM32F1xx / DEVICE_STM32F4xx）支持多芯片平台。
 *          提供了 Flash 解锁/上锁、等待延时校准、缓冲区写入/校验、页/扇区擦除等基础操作，
 *          并内置了可选的 IWDG 看门狗喂狗机制（FEED_IWDG_AUTO）。
 * 
 * @note    页写入结构体可通过 CUS_FLASH_USE_DYNAMIC_PAGE 选择动态或静态分配，
 *          静态分配仅占用一份 RAM 且为单例。
 * 
 * @warning 所有写入或擦除操作前必须确保目标地址已被解锁且处于空闲状态，
 *          操作完成后必须调用 Cus_Flash_Lock 上锁保护 (这通常已在内部自主完成)。
 */
#ifndef __CUS_FALSH_H__
#define __CUS_FALSH_H__


/* ****************************************************** */
  #define DEVICE_STM32F1xx          (1)
  #define DEVICE_STM32F4xx          (0)


/* ****************************************************** */


#if (DEVICE_STM32F1xx)
  #include "stm32f1xx_hal.h"
#elif (DEVICE_STM32F4xx)
  #include "stm32f4xx_hal.h"
#endif 

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


typedef enum Cus_Flash_State
{
  CUS_FLASH_OK,
  CUS_FLASH_TIMEOUT,
  CUS_FLASH_BUSY,
  CUS_FLASH_ERROR,
  CUS_FLASH_PARAMERR,
  CUS_FLASH_PROGRAM_WRPRTERR,
  CUS_FLASH_PROGRAM_PGERR

} Cus_Flash_State_t;


/* *********************** Define ************************ */
  #if (DEVICE_STM32F1xx)
    #define SYSTEMCLOCK_24Mhz           (24000000UL)
    #define SYSTEMCLOCK_48Mhz           (48000000UL)
    #define SYSTEMCLOCK_72Mhz           (72000000UL)
    #define FLASH_SIZE_REG              (*((uint16_t *)0x1FFFF7E0UL)) 
  #endif // DEVICE_STM32F1xx

  #if (DEVICE_STM32F4xx)
    #define SYSTEMCLOCK_30Mhz           (30000000UL)
    #define SYSTEMCLOCK_60Mhz           (60000000UL)
    #define SYSTEMCLOCK_90Mhz           (90000000UL)
    #define SYSTEMCLOCK_120Mhz          (120000000UL)
    #define SYSTEMCLOCK_150Mhz          (150000000UL)
    #define SYSTEMCLOCK_168Mhz          (168000000UL)
    #define FLASH_SIZE_REG              (*((uint16_t *)0x1FFF7A22UL))
  #endif // DEVICE_STM32F4xx

  #define FLASH_KEYR_KEY1               (0x45670123UL)
  #define FLASH_KEYR_KEY2               (0xCDEF89ABUL)
  #define FLASH_ERASE_TIMEOUT_MS        (100U)
  #define FLASH_SIZE_BYTES              (FLASH_SIZE_REG * 1024UL)
  #define FLASH_END_ADDR                (FLASH_BASE + FLASH_SIZE_BYTES)
/* ******************************************************* */

/* *********************** Config ************************ */
  #define FEED_IWDG_AUTO                (1)     // 1=使用IWDG.耗时操作时将主动喂狗. 0=不使用IWDG.程序逻辑不负责喂狗.
  #define CUS_FLASH_USE_DYNAMIC_PAGE    (0)     // 1=动态分配. 0=静态分配.

  #if defined(FLASH_TYPEERASE_PAGES)
    #define FLASH_BYTES_PER_PAGE        (2048U)
  #endif 
/* ******************************************************* */

/* ----------------------------------------------------------- */
#if defined(FLASH_TYPEERASE_PAGES) 

/**
 * @brief Flash 页操作控制块
 * @note  通过 Factory_GetPageControlBlock 获取，使用完后需调用 Release 释放。
 *        可使用 Reset 清空内容后复用。
*/
  typedef struct Cus_Flash_Page Cus_Flash_Page_t;
  struct Cus_Flash_Page
  {
    uint8_t PageDataBuffer[FLASH_BYTES_PER_PAGE];
    uint32_t PageAddress;

    void (*Reset)( Cus_Flash_Page_t *pPage );
    void (*Release)( Cus_Flash_Page_t *pPage );
  };


  Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress);
  uint16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount );
  uint32_t Cus_Flash_GetPageAddress( uint32_t page_index );
  uint32_t Cus_Flash_GetPageStart( uint32_t Address );
  int16_t Cus_Flash_GetPageIndex( uint32_t Address );
  uint16_t Cus_Flash_GetTotalPages( void );
  int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress );
  Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_Page_t *pPage );
  Cus_Flash_State_t Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut );
  bool Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size );


  void Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage );
  void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage );

#endif // FLASH_TYPEERASE_PAGES

#if defined(FLASH_TYPEERASE_SECTORS)

#endif // FLASH_TYPEERASE_SECTORS
/* ----------------------------------------------------------- */



/* ----------------------------------------------------------- */
  bool Cus_Flash_CalibrateLatency( void );
  void Cus_Flash_Unlock( void );
  void Cus_Flash_Lock( void );
  Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size );
  bool Cus_Flash_VerifyBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Size );
  bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size );


  void Cus_FLASH_UnlockFailed_Hook( void );
  void Cus_FLASH_LockFailed_Hook( void );
  void Cus_FLASH_EraseFailed_Hook( void );
  void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize );
/* ----------------------------------------------------------- */


#endif // __CUS_FALSH_H__
