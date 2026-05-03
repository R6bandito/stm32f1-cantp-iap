#include "main.h"


/* ---------------------------------------------- */
extern volatile BL_State_t g_bootloaderState;
Cus_Flash_Page_t *pPage;
static bool is_Retry = false;

#if (USE_POWER_FAIL_RESUME)
  extern Bootloader_record_t recordStructure;
#endif // USE_POWER_FAIL_RESUME
/* ---------------------------------------------- */


int main( void )
{
	Cus_Bootloader_Init();

	printf("Project Test OK!\nsystemCoreClock = :%u\n", SystemCoreClock);

	printf(" ============== Bootloader Running ================= \n");

	g_bootloaderState = BL_STATE_START;

	HAL_Delay(500);

	#if (USE_RECOVERY_APP)
		uint32_t bootCount = Cus_Bootloader_GetBootCount();
		if ( bootCount >= MAX_FAILED_COUNT )
		{
			// 连续重启超过阈值，APP 反复崩溃，且DOWNLOAD区可能已损坏.放弃主流程，直接进恢复区.
			Cus_Bootloader_ClearBootCount();
			Cus_Bootloader_JumpToRecoveryAPP();
			for( ; ; );			// 兜底. 程序不应该执行到这里.
		}

		Cus_Bootloader_IncreaseBootCount();		// 本次启动计数 + 1.(请在APP成功跳转后进行清除)
	#endif // USE_RECOVERY_APP

	uint8_t uReturn = Cus_Bootloader_CheckIAPRequest();
	if ( !uReturn )		
	{
		// No need to update. Jump to the APP.
		g_bootloaderState = BL_STATE_JUMP_APP;
	} 
	else 
	{
		g_bootloaderState = BL_STATE_ERASE_APP;
	}

	#if (USE_POWER_FAIL_RESUME)
		uint8_t hReturn = Cus_Bootloader_PowerFailResume_GetAllInfoFromBKPField();		// 获取BKP内存储的烧写信息.
		if ( !recordStructure.error_flag && hReturn )
		{
			// 无错误状态. 状态更新 + 跳转.
			g_bootloaderState = recordStructure.record_state;
		}
		else if ( recordStructure.error_flag && hReturn )
		{
			#if (PWRFAIL_CONF_IGNORE_ERROR) == 1
				g_bootloaderState = recordStructure.record_state;		// 无视错误状态. 依然走断电续写流程.
			#elif (PWRFAIL_CONF_IGNORE_ERROR) == 0
				// 有错误状态. 且配置为不忽略错误. 重走升级流程,清除BKP相关区域.
				Cus_Bootloader_PowerFailResume_ResetBKPField();
			#endif // PWRFAIL_CONF_IGNORE_ERROR
		}
	#endif // USE_POWER_FAIL_RESUME

	// 固件更新信息.
	uint32_t writeSize = ((IAP_Info_t *)IAP_INFO_STRUCT_START_ADDR)->app_size;
	uint32_t total_pages = (writeSize / (SIZE_PER_PAGE_KB * 1024));
	uint32_t remaining = (writeSize % (SIZE_PER_PAGE_KB * 1024));

	Factory_GetPageControlBlock(&pPage);

	while(1)
	{
		Cus_Bootloader_FeedIWDG();

		switch (g_bootloaderState)
		{
			case BL_STATE_ERASE_APP:
			{
				#if (USE_POWER_FAIL_RESUME)
					Cus_Bootloader_PowerFailResume_SaveStates(BL_STATE_ERASE_APP);	// 记录 BL_STATE_ERASE_APP 状态.
				#endif 

				uint32_t page_size = SIZE_PER_PAGE_KB * 1024;
				uint32_t erase_count = (APP_REGION_SIZE + page_size - 1) / page_size;		// 向上取整.防止因APP START地址问题导致漏擦除.
				uint16_t SuccessErasePages = Cus_Flash_ErasePages(APP_START_ADDRESS, erase_count);
				if ( SuccessErasePages != erase_count )
				{
					Cus_BootloaderHook_EraseFailed( (APP_START_ADDRESS + (SuccessErasePages * 1024)), CUS_FLASH_ERROR );
					for( ; ; );
				}
				g_bootloaderState = BL_STATE_WRITE_FW;
				#if (USE_POWER_FAIL_RESUME)
					Cus_Bootloader_PowerFailResume_SaveStates(BL_STATE_WRITE_FW);  // 记录 BL_STATE_WRITE_FW 状态.
				#endif 

				break;
			}

			case BL_STATE_WRITE_FW:
			{
				static uint16_t current_pages = 0;
				static uint32_t current_downloadAddr = DOWNLOAD_START_ADDRESS;
				static uint32_t current_appAddr = APP_START_ADDRESS;

				#if (USE_POWER_FAIL_RESUME)
					static uint8_t resume_initialize = 0;
					if ( !resume_initialize )
					{
						Cus_Bootloader_PowerFailResume_ReloadWriteParams(&current_pages, &current_downloadAddr, &current_appAddr);
						printf("\nPages: %d, downloadAddr: %x, appAddr: %x\n", current_pages, current_downloadAddr, current_appAddr);
						resume_initialize = 1;		// 不可重入标志.
					}
				#endif 

				if ( is_Retry )
				{
					current_pages = 0;
					current_downloadAddr = DOWNLOAD_START_ADDRESS;
					current_appAddr = APP_START_ADDRESS;
				}

				pPage->Reset(pPage);

				pPage->PageAddress = current_appAddr;
				memcpy(pPage->PageDataBuffer, (uint8_t *)current_downloadAddr, (SIZE_PER_PAGE_KB * 1024));
				Cus_Bootloader_FeedIWDG();

				Cus_Flash_State_t hReturn = Cus_Flash_WritePage(pPage);
				Cus_Bootloader_FeedIWDG();
				if ( hReturn != CUS_FLASH_OK )
				{
					Cus_BootloaderHook_WriteFailed(pPage->PageAddress, hReturn);
					for( ; ; );								// 写入失败. 由于已经擦除APP区. 此处失败后进入死循环. 待下一步处理.
				}
				#if (USE_POWER_FAIL_RESUME)
					Cus_Bootloader_PowerFailResume_PagesIncrease();		// 成功写入一页，BKP对应记录器++.
				#endif 

				/* ---------- 测试 ------------------- */
				#if (RELEASE) == 0
					static uint8_t test = 0;
					test++;
					if ( (test == 3) && (*(volatile uint16_t *)(0x40006C28)) != 1 )
					{
						*(volatile uint16_t *)(0x40006C28) = 1;
						printf("\nPOWER_FIAL_RESUME_START_IN_5_SEC.\n");
						HAL_Delay(5000);
						NVIC_SystemReset();
					}
				#endif 
			/* ---------------------------------- */

				current_pages++;	
				current_downloadAddr += (SIZE_PER_PAGE_KB * 1024);		// 偏移到下个待读取的页.
				current_appAddr += (SIZE_PER_PAGE_KB * 1024);					// 偏移到下一个待写入的页.

				if ( current_pages == total_pages )
				{	
					// 整数页已经写入完. 
					if ( remaining == 0 )
					{
						// 无剩余不到一个页大小的数据.
						g_bootloaderState = BL_STATE_VERIFY_FW;
						pPage->Release(pPage);

						#if (USE_POWER_FAIL_RESUME)
							Cus_Bootloader_PowerFailResume_SaveStates(BL_STATE_VERIFY_FW);
						#endif 
						break;	
					}

					if ( remaining != 0 && remaining < (SIZE_PER_PAGE_KB * 1024) )
					{
						// 剩余数据不足一页.
						Cus_Bootloader_FeedIWDG();
						memset(pPage->PageDataBuffer, 0, (SIZE_PER_PAGE_KB * 1024));
						memcpy(pPage->PageDataBuffer, (uint8_t *)current_downloadAddr, remaining);
						pPage->PageAddress = current_appAddr;

						Cus_Flash_State_t hReturn = Cus_Flash_WritePage(pPage);
						Cus_Bootloader_FeedIWDG();
						if ( hReturn != CUS_FLASH_OK )
						{
							Cus_BootloaderHook_WriteFailed(pPage->PageAddress, hReturn);
							for( ; ; );
						}

						pPage->Release(pPage);
						g_bootloaderState = BL_STATE_VERIFY_FW;

						#if (USE_POWER_FAIL_RESUME)
							Cus_Bootloader_PowerFailResume_SaveStates(BL_STATE_VERIFY_FW);
						#endif 
						break;
					}
				}
				continue;
			}

		case BL_STATE_VERIFY_FW:
			{
				static uint8_t retry_count = 0;
				bool is_FW_VerifyOK = Cus_Flash_VerifyBuffer(APP_START_ADDRESS, (uint8_t *)DOWNLOAD_START_ADDRESS, writeSize);
				Cus_Bootloader_FeedIWDG();
				if ( !is_FW_VerifyOK )
				{
					// Frameware Verify Failed. Start Retry.
					retry_count++;
					if ( retry_count <= 3 )
					{
						is_Retry = true;

						#if (USE_POWER_FAIL_RESUME)
							Cus_Bootloader_PowerFailResume_ResetBKPField();		// Retry 前清除此前计数. 保证BKP记录与Bootloader一致.
						#endif 

						g_bootloaderState = BL_STATE_ERASE_APP;		// Back to BL_STATE_ERASE_APP.
						break;
					}
					else 
					{
						// 重试 3 次全部失败，触发 VerifyFailed Hook（默认软复位）.
						Cus_BootloaderHook_VerifyFailed(APP_START_ADDRESS, writeSize);
					}
					for( ; ; );
				}
				
				is_Retry = false;
				retry_count = 0;
				g_bootloaderState = BL_STATE_CLEAR_IAP_FLAG;

				#if (USE_POWER_FAIL_RESUME)
					Cus_Bootloader_PowerFailResume_SaveStates(BL_STATE_CLEAR_IAP_FLAG);
				#endif 
				break;
			}

		case BL_STATE_CLEAR_IAP_FLAG:
			{
				Cus_Bootloader_FeedIWDG();
				uint32_t IAP_Info_Page = Cus_Flash_GetPageStart(IAP_INFO_STRUCT_START_ADDR);
				
				Cus_Flash_State_t eReturn = Cus_Flash_ErasePage(IAP_Info_Page);
				Cus_Bootloader_FeedIWDG();
				if ( eReturn != CUS_FLASH_OK )
				{
					Cus_BootloaderHook_EraseFailed(IAP_INFO_STRUCT_START_ADDR, eReturn);
					for( ; ; );
				}

				g_bootloaderState = BL_STATE_JUMP_APP;
				#if (USE_POWER_FAIL_RESUME)
					Cus_Bootloader_PowerFailResume_SaveStates(BL_STATE_JUMP_APP);
				#endif 				
				break;
			}

		case BL_STATE_JUMP_APP:
			{
				// Jump to APP.
				Cus_Bootloader_FeedIWDG();

				uint32_t msp = *(volatile uint32_t *)APP_START_ADDRESS;		// 读取栈顶地址.
				if ( msp < MCU_SRAM_BASE_ADDR || msp > (MCU_SRAM_BASE_ADDR + MCU_SRAM_SIZE) )
				{
					// 栈顶地址非法.
					#if (USE_RECOVERY_APP)
						Cus_Bootloader_JumpToRecoveryAPP();	// 栈顶非法. 由于擦写和校验已成功，此处却栈顶出现问题. 极大可能DOWNLOAD区与APP区已经损毁. 开启USE_RECOVERY_APP功能的情况下直接跳转.
					#else 
						// 不启用恢复区，走通用错误 Hook（默认软复位）.
						Cus_BootloaderHook_GenericError(BL_STATE_JUMP_APP, IAP_ERRCODE_INVALID_STACKTOPADDR);
					#endif // USE_RECOVERY_APP

					for( ; ; );
				}

				uint32_t reset_vector = *(volatile uint32_t *)(APP_START_ADDRESS + 4);
				if ( reset_vector < APP_START_ADDRESS || reset_vector > (APP_START_ADDRESS + APP_REGION_SIZE) )
				{
					// 复位向量地址非法.
					#if (USE_RECOVERY_APP)
						Cus_Bootloader_JumpToRecoveryAPP();
					#else 
						Cus_BootloaderHook_GenericError(BL_STATE_JUMP_APP, IAP_ERRCODE_INVALID_RESETHANDLER);
					#endif // USE_RECOVERY_APP

					for( ; ; );
				}

				#if (USE_POWER_FAIL_RESUME)
					Cus_Bootloader_PowerFailResume_ResetBKPField();		// 最后跳转前彻底清除断电续传标志.
				#endif 

				SysTick->CTRL = 0;          // 跳转前彻底关闭 SysTick，防止中断残留.
				SysTick->LOAD = 0;
				SysTick->VAL  = 0;
				
				SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;		// 清除可能已经挂起的 SysTick 中断请求.

				__disable_irq();
				HAL_DeInit();
				SCB->VTOR = APP_START_ADDRESS;		// 设置中断向量表.
				__DSB();
				__set_MSP(msp);							// 设置主堆栈.

				void (*app_entry)(void) = (void (*)(void))reset_vector;

				app_entry();

				for( ; ; );			// 正常情况下. 程序不应该运行到这里.
				break;
			}
		
		default:	break;
		}
	}
}

