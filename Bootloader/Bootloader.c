#include "Bootloader.h"


/* ------------------- g_ver --------------------- */

volatile BL_State_t g_bootloaderState;

#if (USE_POWER_FAIL_RESUME)
  Bootloader_record_t recordStructure;
#endif // USE_POWER_FAIL_RESUME

/* ---------------------------------------------- */


/* ---------------------------------------------- */
uint8_t Cus_Bootloader_CheckIAPRequest( void );
void Cus_Bootloader_Init( void );

static uint8_t Cus_Bootloader_CRC32Verify( uint32_t exptected_CRC );
static uint32_t Cus_Bootloader_CRC32Caculate( uint8_t *pData, uint32_t data_len );
/* ---------------------------------------------- */


void Cus_Bootloader_Init( void )
{
  HAL_Init();

  #if (USE_UTILS_SYSCONF)
    Cus_Bootloader_Utils_SystemClockConfig();
  #else
    #warning "User must provide a Custom SystemConfig API to Initialize the SystemClock if you dont use the default."
    {
      // Your API Called at Here.
      // e.g. Custom_SystemClockConfig();
    }
  #endif // USE_UTILS_SYSCONF
  
  #if (USE_UTILS_DEBUG)
    Cus_Bootloader_Utils_DebugUART();
  #endif // USE_UTILS_DEBUG

  #if (USE_RECOVERY_APP)
    Cus_Bootloader_RecoveryInit();
  #endif // USE_RECOVERY_APP

  #if (USE_POWER_FAIL_RESUME)
    Cus_Bootloader_PowerFailResume_RecordInit();
  #endif // USE_POWER_FAIL_RESUME

  Cus_Bootloader_FeedIWDG();

  Cus_Flash_CalibrateLatency();
}


uint8_t Cus_Bootloader_CheckIAPRequest( void )
{
  IAP_Info_t *iap_info = (IAP_Info_t *)IAP_INFO_STRUCT_START_ADDR;

  g_bootloaderState = BL_STATE_CHECK_FLAG;
  if ( iap_info->magic_word != IAP_MAGIC_WORD )   return 0;     // No IAP Update Request.

  if ( iap_info->app_size > DOWNLOAD_REGION_SIZE || iap_info->app_size == 0 )  return 0;   // Size Error.

  g_bootloaderState = BL_STATE_VERIFY_CRC;
  uint8_t CRC_CheckReturn = Cus_Bootloader_CRC32Verify(iap_info->CRC32);
  if ( !CRC_CheckReturn )   
  {
    // CRC Verify Failed! Fireware not reliable.Discard this update required.
    uint32_t IAP_Addr_Page = Cus_Flash_GetPageStart(IAP_INFO_STRUCT_START_ADDR);
    Cus_Flash_State_t hReturn = Cus_Flash_ErasePage(IAP_Addr_Page);
    if ( hReturn != CUS_FLASH_OK )
    {
      Cus_BootloaderHook_EraseFailed( IAP_INFO_STRUCT_START_ADDR, hReturn );
    }
    return 0;
  }

  // CRC Verify Success.
  return 1;
}


static uint8_t Cus_Bootloader_CRC32Verify( uint32_t exptected_CRC )
{
  IAP_Info_t *iap_info = (IAP_Info_t *)IAP_INFO_STRUCT_START_ADDR;

  uint32_t CalculateCRC = Cus_Bootloader_CRC32Caculate((uint8_t *)DOWNLOAD_START_ADDRESS, iap_info->app_size);

  Cus_Bootloader_FeedIWDG();

  if ( CalculateCRC != exptected_CRC )  return 0;   // CRC Verify Failed!

  return 1;   
}


static uint32_t Cus_Bootloader_CRC32Caculate( uint8_t *pData, uint32_t data_len )
{
  uint32_t crc = 0xFFFFFFFF;    // CRC 初始值.
  const uint32_t *table = crc32_table;

  for( uint32_t i = 0; i < data_len; i++ )
  {
    crc = (crc >> 8) ^ table[(crc ^ pData[i]) & 0xFF];

    if ( i % 1024 == 0 )
    {
      Cus_Bootloader_FeedIWDG();    // 每1024字节喂狗一次.
    }
  }

  return crc ^ 0xFFFFFFFF; 
}



void Cus_Bootloader_FeedIWDG( void )
{
  #if (USE_IWDG)
    uint16_t Reload = 0xAAAAUL;

    IWDG->KR = (Reload & 0xFFFFUL);
  #endif
  __nop();
}



/* ****************************** Options: Revocery ******************************************* */
  #if (USE_RECOVERY_APP)

    void Cus_Bootloader_RecoveryInit( void )
    {
      // 解除 BKP 寄存器写保护.
      uint32_t rcc_temp = RCC->APB1ENR;
      rcc_temp |= (0x01UL << 28);         // PWREN 置 1.
      rcc_temp |= (0x01UL << 27);         // BKPEN 置 1.
      RCC->APB1ENR = rcc_temp;

      uint32_t pwr_temp = PWR->CR;
      pwr_temp |= (0x01UL << 8);          // DBP 置 1.
      PWR->CR = pwr_temp;
    }


    uint32_t Cus_Bootloader_GetBootCount( void )
    {
      uint32_t bkp_1registerAddr = RECOVERY_BKP_ADDR;

      // 魔数不匹配 = 首次上电或掉电过，计数归零
      if ( *(volatile uint32_t *)bkp_1registerAddr != RECOVERY_BKP_MAGIC )  return 0;  // 魔数校验失败.返回 0 表示正常启动

      uint32_t bkp_2registerAddr = (RECOVERY_BKP_ADDR + 0x04UL);        // 第二个 BKP 寄存器.
      return *(volatile uint32_t *)bkp_2registerAddr;
    }


    void Cus_Bootloader_IncreaseBootCount( void )
    {
      uint32_t bkp_1registerAddr = RECOVERY_BKP_ADDR;
      if ( *(volatile uint32_t *)bkp_1registerAddr != RECOVERY_BKP_MAGIC )  
      {
        // 首次写入时，写入魔数验证码进行初始化.
        uint32_t *bkp_1registerpAddr = (volatile uint32_t *)(RECOVERY_BKP_ADDR + 0x00UL);  // 首个BKP寄存器.
        *bkp_1registerpAddr = RECOVERY_BKP_MAGIC;
      }

      uint32_t bkp_2registerAddr = (RECOVERY_BKP_ADDR + 0x04UL);
      *(volatile uint32_t *)bkp_2registerAddr += 1;
    }


    void Cus_Bootloader_ClearBootCount( void )
    {
      uint32_t bkp_1registerAddr = RECOVERY_BKP_ADDR;
      if ( *(volatile uint32_t *)bkp_1registerAddr != RECOVERY_BKP_MAGIC  )   return;

      *(volatile uint32_t *)bkp_1registerAddr = 0x00;   // 清除魔数.

      uint32_t bkp_2registerAddr = (RECOVERY_BKP_ADDR + 0x04UL);
      *(volatile uint32_t *)bkp_2registerAddr = 0x00;   // 清除数据.
    }


    void Cus_Bootloader_JumpToRecoveryAPP( void )     // 此处为便于独立逻辑，并未将其与main中的Jump逻辑进行整合合并为统一接口.
    {
      Cus_Bootloader_FeedIWDG();

      uint32_t recovery_msp = *(volatile uint32_t *)RECOVERY_APP_START_ADDR;    // 读取栈顶.
      if ( recovery_msp < MCU_SRAM_BASE_ADDR || recovery_msp > MCU_SRAM_BASE_ADDR + MCU_SRAM_SIZE )
      {
        // 最后恢复区APP栈顶地址非法.说明该区域从未被烧录或已损坏.
        // 此时三层固件（APP、DOWNLOAD、RECOVERY）全部失效.
        // 打印最后的诊断信息.
        #if (USE_UTILS_DEBUG)
          printf("========================================\n");
          printf("\n=== FATAL: ALL FIRMWARE CORRUPTED ===\n");
          printf("Recovery APP stack top: 0x%08X (invalid)\n", recovery_msp);
          printf("Device halted. Re-program required.\n");
          printf("========================================\n");
        #endif


        for( ; ; )    // 死循环兜底.
        {
          #if (USE_IWDG)
            Cus_Bootloader_FeedIWDG();    // 喂狗防止复位(APP DOWNLOAD RESCUE APP区均已出错. 再复位已无意义).
            HAL_Delay(5);   
          #endif // USE_IWDG
        }   
      }

      uint32_t recovery_reset_vector = *(volatile uint32_t *)(RECOVERY_APP_START_ADDR + 0x04UL);    // 取出复位向量.
      if ( recovery_reset_vector < RECOVERY_APP_START_ADDR || recovery_reset_vector > RECOVERY_APP_START_ADDR + RECOVERY_APP_REGION_SIZE )
      {
        #if (USE_UTILS_DEBUG)
          printf("========================================\n");
          printf(" \nFATAL ERROR: ALL FIRMWARE CORRUPTED\n");
          printf("Bootloader: OK (0x%08X)\n", BOOTLOADER_START_ADDRESS);
          printf("APP:        CORRUPTED\n");
          printf("Download:   CORRUPTED or EMPTY\n");
          printf("Recovery:   CORRUPTED or NOT PROGRAMMED\n");
          printf("Recovery reset vector: 0x%08X (invalid)\n", recovery_reset_vector);
          printf("\nDevice halted. Please re-program via debugger.\n");
          printf("========================================\n");
        #endif

        for( ; ; )    // 死循环兜底.
        {
          #if (USE_IWDG)
            Cus_Bootloader_FeedIWDG();    // 喂狗防止复位(APP DOWNLOAD RESCUE APP区均已出错. 再复位已无意义).
            HAL_Delay(5);   
          #endif // USE_IWDG
        }   
      }

      SysTick->CTRL = 0;          // 跳转前彻底关闭 SysTick，防止中断残留.
      SysTick->LOAD = 0;
      SysTick->VAL  = 0;
      SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;		// 清除可能已经挂起的 SysTick 中断请求.

      __disable_irq();
      HAL_DeInit();
      SCB->VTOR = RECOVERY_APP_START_ADDR;
      __DSB();
      __set_MSP(recovery_msp);

      Cus_Bootloader_FeedIWDG();    // 跳转前最后一次喂狗.
      void (*reset_entry)(void) = (void (*)(void))recovery_reset_vector;
      reset_entry();

      for( ; ; );   // 程序不应该执行到这里.
    }

  #endif // USE_RECOVERY_APP
/* ******************************************************************************************** */


/* ****************************** Options: POWER_FAIL_RESUME ******************************************* */
  #if (USE_POWER_FAIL_RESUME)

    void Cus_Bootloader_PowerFailResume_RecordInit( void )
    {
      uint32_t rcc_temp = RCC->APB1ENR;
      uint32_t pwr_temp = PWR->CR;
      if ( (rcc_temp & (0x01UL << 27)) != 1 || (rcc_temp & (0x01UL << 28)) != 1 || (pwr_temp & (0x01UL << 8)) != 1 )
      {
        // BKP域未开启访问. 将其开启.
        rcc_temp |= (0x01UL << 28);
        rcc_temp |= (0x01UL << 27);
        RCC->APB1ENR = rcc_temp;

        pwr_temp |= (0x01UL << 8);
        PWR->CR = pwr_temp;
      }
    }


    void Cus_Bootloader_PowerFailResume_PagesIncrease( void )
    {
      if ( *(volatile uint16_t *)RESUME_BKP_MAGIC_ADDR != PWRFAIL_RESUME_BKP_MAGIC )   
      {
        // 第一次写入时再写入魔数.
        *(volatile uint16_t *)RESUME_BKP_MAGIC_ADDR = PWRFAIL_RESUME_BKP_MAGIC;  // 魔数写入.
      }

      uint16_t pages = *(volatile uint16_t *)RESUME_BKP_PAGE_ADDR;
      pages++;
      *(volatile uint16_t *)RESUME_BKP_PAGE_ADDR = pages;
      __DSB();

      if ( *(volatile uint16_t *)RESUME_BKP_PAGE_ADDR != pages )
      {
        // 写入后回读验证失败.(极小概率)
        // 置错误位. 代表当前数据不可信.
        Cus_Bootloader_PowerFailResume_SetError();
      }
    }


    void Cus_Bootloader_PowerFailResume_SetError( void )
    {
      if ( *(volatile uint16_t *)RESUME_BKP_MAGIC_ADDR != PWRFAIL_RESUME_BKP_MAGIC )    return;   // 魔数验证失败,空调用.

      uint16_t error_bkp = RESUME_BKP_ERROR_FLAG_ADDR;
      *(volatile uint16_t *)error_bkp = 0x01;   // 此处不验证，因为此前已处于异常路径.
    }


    void Cus_Bootloader_PowerFailResume_SaveStates( BL_State_t state )
    {
      *(volatile uint16_t *)RESUME_BKP_STATE_ADDR = (uint16_t)state;
      __DSB();

      if ( *(volatile uint16_t *)RESUME_BKP_STATE_ADDR != (uint16_t)state )
      {
        Cus_Bootloader_PowerFailResume_SetError();
      }
    }

    static inline BL_State_t Cus_Bootloader_PowerFailResume_ToBLState( uint16_t val ) 
    {
      return (val <= BL_STATE_JUMP_APP) ? (BL_State_t)val : BL_STATE_START;
    }


    uint8_t Cus_Bootloader_PowerFailResume_GetAllInfoFromBKPField( void )
    {
      if ( *(volatile uint16_t *)RESUME_BKP_MAGIC_ADDR != PWRFAIL_RESUME_BKP_MAGIC )    return 0;   // 魔数验证失败,空调用.

      recordStructure.record_magic = *(volatile uint16_t *)RESUME_BKP_MAGIC_ADDR;
      recordStructure.record_pages = *(volatile uint16_t *)RESUME_BKP_PAGE_ADDR;;
      recordStructure.error_flag = (uint8_t)(*(volatile uint16_t *)RESUME_BKP_ERROR_FLAG_ADDR & 0x01);

      BL_State_t Cvstate = Cus_Bootloader_PowerFailResume_ToBLState( *(volatile uint16_t *)RESUME_BKP_STATE_ADDR );
      recordStructure.record_state = Cvstate;

      return 1;
    }


    void Cus_Bootloader_PowerFailResume_ResetBKPField( void )
    {
      *(volatile uint16_t *)RESUME_BKP_MAGIC_ADDR      = 0x00;
      *(volatile uint16_t *)RESUME_BKP_STATE_ADDR      = 0x00;
      *(volatile uint16_t *)RESUME_BKP_PAGE_ADDR       = 0x00;
      *(volatile uint16_t *)RESUME_BKP_ERROR_FLAG_ADDR = 0x00;
    }


    void Cus_Bootloader_PowerFailResume_ReloadWriteParams( uint16_t *p_current_pages, uint32_t *p_current_downloadAddr, uint32_t *p_current_appAddr )
    {
      // 该方法只初始化一次. 不允许重入！（无论是正常上电还是断电续传）. 由上层保证.
      if ( !p_current_appAddr || !p_current_downloadAddr || !p_current_pages )  return;

      if ( recordStructure.record_magic != PWRFAIL_RESUME_BKP_MAGIC )   return; 

      #if (PWRFAIL_CONF_IGNORE_ERROR) == 0
        if ( recordStructure.error_flag != 0 )  return;
      #endif 

      const IAP_Info_t *p_IAP_info = (const IAP_Info_t *)IAP_INFO_STRUCT_START_ADDR;
      if ( p_IAP_info->magic_word != IAP_MAGIC_WORD )   return;   // IAP信息有误.(意外擦除, 损毁). (一般不应发生这种情况，此处仅作保留)

      *p_current_pages = recordStructure.record_pages;
      *p_current_downloadAddr = DOWNLOAD_START_ADDRESS + (recordStructure.record_pages * (SIZE_PER_PAGE_KB * 1024));  // 按页进行偏移.
      *p_current_appAddr = APP_START_ADDRESS + (recordStructure.record_pages * (SIZE_PER_PAGE_KB * 1024));

      if ( *p_current_downloadAddr < DOWNLOAD_START_ADDRESS || *p_current_downloadAddr > DOWNLOAD_START_ADDRESS + DOWNLOAD_REGION_SIZE || 
            *p_current_appAddr < APP_START_ADDRESS || *p_current_appAddr > APP_START_ADDRESS + APP_REGION_SIZE )
      {
        // 计算出来的位置错误？擦除整个已写APP. 降级为普通模式，从头开始擦写.
        uint32_t page_size = SIZE_PER_PAGE_KB * 1024;
				uint32_t erase_count = (APP_REGION_SIZE + page_size - 1) / page_size;		// 向上取整.防止因APP START地址问题导致漏擦除.
				uint16_t SuccessErasePages = Cus_Flash_ErasePages(APP_START_ADDRESS, erase_count);
        Cus_Bootloader_FeedIWDG();
				if ( SuccessErasePages != erase_count )
				{
					Cus_BootloaderHook_EraseFailed( (APP_START_ADDRESS + (SuccessErasePages * 1024)), CUS_FLASH_ERROR );
					for( ; ; );
				}

        // 降级为默认配置.
        *p_current_downloadAddr = DOWNLOAD_START_ADDRESS;
        *p_current_appAddr = APP_START_ADDRESS;
        *p_current_pages = 0;

        Cus_Bootloader_PowerFailResume_ResetBKPField();   // 降级处理后清除BKP信息. 下次启动视为全新启动.
      }
    }

  #endif // USE_POWER_FAIL_RESUME
/* ******************************************************************************************** */


/* ****************************** Hook Default ******************************************* */

__weak void Cus_BootloaderHook_EraseFailed( uint32_t page_addr, Cus_Flash_State_t error )
{
  UNUSED(page_addr);
  UNUSED(error);

  #if (USE_UTILS_DEBUG)
    printf("Cus_BootloaderHook_FailedToErase Trigged!\n\n");
    printf("[BOOT] Erase Failed! Addr:0x%08X, Err:%d. System will reset in 3s...\n", page_addr, error);
  #endif

  HAL_Delay(500);
  NVIC_SystemReset();
}


__weak void Cus_BootloaderHook_WriteFailed( uint32_t target_addr, Cus_Flash_State_t error )
{
  UNUSED(target_addr);
  UNUSED(error);

  #if (USE_UTILS_DEBUG)
    printf("Cus_BootloaderHook_WriteFWFailed Trigged!\n\n");
    printf("[BOOT] Write Failed! Addr:0x%08X, Err:%d. System will reset in 3s...\n", target_addr, error);
  #endif  

  HAL_Delay(500);
  NVIC_SystemReset();
}


__weak void Cus_BootloaderHook_VerifyFailed( uint32_t region_start, uint32_t size )
{
  UNUSED(region_start);
  UNUSED(size);

  #if (USE_UTILS_DEBUG)
    printf("Cus_BootloaderHook_VerifyFailed Trigged!\n\n");
    printf("[BOOT] Verify Failed! Region:0x%08X, Size:%u. System will reset in 3s...\n", region_start, size);
  #endif  

  HAL_Delay(500);
  NVIC_SystemReset();
}


__weak void Cus_BootloaderHook_GenericError( BL_State_t state, uint32_t error_code )
{
  UNUSED(state);
  UNUSED(error_code);

  #if (USE_UTILS_DEBUG)
    printf("Cus_BootloaderHook_GenericError Trigged!\n\n");
    printf("[BOOT] Generic Error! State:%d, Code:0x%02X. System will reset in 3s...\n", state, error_code);
  #endif  

  HAL_Delay(500);
  NVIC_SystemReset();
}

/* ***************************************************************************************** */

