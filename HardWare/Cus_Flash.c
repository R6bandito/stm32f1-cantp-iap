#include "Cus_Flash.h"



/* ****************************************************** */
#if defined(FLASH_TYPEERASE_PAGES) && (CUS_FLASH_USE_DYNAMIC_PAGE == 0)
  static bool g_is_PageAllocated = false;
#endif
/* ****************************************************** */


/* ****************************************************** */

bool Cus_Flash_CalibrateLatency( void );
void Cus_Flash_Unlock( void );
void Cus_Flash_Lock( void );
Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size );
bool Cus_Flash_VerifyBuffer(uint32_t StartAddress, uint8_t *pData, uint32_t Size);
bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size );

static uint32_t Cus_Flash_GetSpinCount( uint32_t ms );
static void Cus_Flash_FeedIWDG( void );

#if defined(FLASH_TYPEERASE_PAGES) 
  uint32_t Cus_Flash_GetPageAddress( uint32_t page_index );
  uint32_t Cus_Flash_GetPageStart( uint32_t Address );
  bool Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size );
  Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress);
  uint16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount );
  Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_Page_t *pPage );
  int16_t Cus_Flash_GetPageIndex( uint32_t Address );
  uint16_t Cus_Flash_GetTotalPages( void );
  int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress );
  Cus_Flash_State_t Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut );
  static void PageStructure_Reset( Cus_Flash_Page_t *pPage );
  static void PageStructure_Release( Cus_Flash_Page_t *pPage );

  void Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage );
  void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage );
#endif // FLASH_TYPEERASE_PAGES

void Cus_FLASH_UnlockFailed_Hook( void );
void Cus_FLASH_LockFailed_Hook( void );
void Cus_FLASH_EraseFailed_Hook( void );
void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize );
/* ****************************************************** */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  锁定 Flash 控制器
 * @note   会检查当前是否已上锁，已上锁则直接返回。
 *          上锁失败时会触发 Cus_FLASH_LockFailed_Hook。
 */
void Cus_Flash_Lock( void )
{
  uint32_t FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) != 0 )    return;   // 已经上锁. 直接返回.

  FLASH->CR = (FLASH->CR | (0x01UL << FLASH_CR_LOCK_Pos));

  FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) == 0 )
  {
    // 上锁失败. 依然处于解锁状态.
    Cus_FLASH_LockFailed_Hook();

    return;
  }
}
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  解锁 Flash 控制器
 * @note   会检查当前是否已解锁，已解锁则直接返回。
 *          解锁失败时会触发 Cus_FLASH_UnlockFailed_Hook。
 */
void Cus_Flash_Unlock( void )
{
  uint32_t FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) == 0 )  return;   // FLASH已经被解锁. 直接返回.

  FLASH->KEYR = FLASH_KEYR_KEY1;    // 写入 KEY1.

  FLASH->KEYR = FLASH_KEYR_KEY2;    // 写入 KEY2.

  FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) != 0 )    // 验证是否成功解锁.
  {
    // 解锁失败.
    Cus_FLASH_UnlockFailed_Hook();

    return;
  }
}
/* -------------------------------------------------------------------------------------------------------- */


static void Cus_Flash_FeedIWDG( void )
{
  #if (FEED_IWDG_AUTO)
    uint16_t Reload = 0xAAAAUL;

    IWDG->KR = (Reload & 0xFFFFUL);
  #endif 
  __nop();
}



bool Cus_Flash_CalibrateLatency( void )
{
  uint32_t desire_Latancy = ((FLASH->ACR) & 0x07UL);

  #if (DEVICE_STM32F1xx)
    if ( (SystemCoreClock > 0) && (SystemCoreClock <= SYSTEMCLOCK_24Mhz) )
    {
      if ( desire_Latancy == 0x00 )   return true;
      else  desire_Latancy = 0x00;
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_24Mhz) && (SystemCoreClock <= SYSTEMCLOCK_48Mhz) )
    {
      if ( desire_Latancy == 0x01 )   return true;
      else desire_Latancy = 0x01;
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_48Mhz) && (SystemCoreClock <= SYSTEMCLOCK_72Mhz) )
    {
      if ( desire_Latancy == ((0x01UL) << 1) )  return true;
      else desire_Latancy = ((0x01UL) << 1);
    }
    else 
    {
      return false;               // 时钟错误. 不在范围内.
    }
  #endif // DEVICE_STM32F1xx

  #if (DEVICE_STM32F4xx)
    if ( (SystemCoreClock > 0) && (SystemCoreClock <= SYSTEMCLOCK_30Mhz) )
    {
      if ( desire_Latancy == 0x00 )   return true;
      else desire_Latancy = 0x00;
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_30Mhz) && (SystemCoreClock <= SYSTEMCLOCK_60Mhz) )
    {
      if ( desire_Latancy == 0x01 )   return true;
      else desire_Latancy = 0x01;
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_60Mhz) && (SystemCoreClock <= SYSTEMCLOCK_90Mhz) )
    {
      if ( desire_Latancy == ((0x01UL) << 1) )  return true;
      else desire_Latancy = ((0x01UL) << 1);
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_90Mhz) && (SystemCoreClock <= SYSTEMCLOCK_120Mhz) )
    {
      if ( desire_Latancy == ((0x01UL) << 1) | 0x01UL )  return true;
      else desire_Latancy = ((0x01UL) << 1) | 0x01UL;
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_120Mhz) && (SystemCoreClock <= SYSTEMCLOCK_150Mhz) )
    {
      if ( desire_Latancy == ((0x01UL) << 2) )  return true;
      else desire_Latancy = ((0x01UL) << 2);
    }
    else if ( (SystemCoreClock > SYSTEMCLOCK_150Mhz) && (SystemCoreClock <= SYSTEMCLOCK_168Mhz) )
    {
      if ( desire_Latancy == ((0x01UL) << 2) | 0x01UL )  return true;
      else desire_Latancy = ((0x01UL) << 2) | 0x01UL;
    }
    else 
    {
      return false;       // 时钟错误. 不在范围内.
    }
  #endif // DEVICE_STM32F4xx

  __disable_irq();
  FLASH->ACR = (FLASH->ACR & ~0x07UL) | desire_Latancy;
  __enable_irq();

  return ((FLASH->ACR & 0x07UL) == desire_Latancy);
}


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief 根据系统时钟计算自旋等待的循环次数（粗略估算）
 * @param ms 超时毫秒数
 * @return 循环次数（每个循环约 5~6 个指令周期）
 */
static uint32_t Cus_Flash_GetSpinCount( uint32_t ms )
{
  // 估算每个循环消耗的 CPU 周期数.
  const uint32_t cycles_per_loop = 5;

  uint32_t sysclk = SystemCoreClock;

  uint32_t total_cycles = (ms * sysclk) / 1000;

  return total_cycles / cycles_per_loop;
}
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief 以半字（HalfWord）为单位写入 Flash
 * @param StartAddress 目标起始地址，必须位于 Flash 区域内且半字对齐
 * @param pData        源数据指针
 * @param Buffer_Size  源数据长度（字节）
 * @return Cus_Flash_State_t 操作状态
 * 
 * @note  写入前必须确保目标区域已被擦除（全 0xFF），否则会产生写入错误。
 *        若长度是奇数，剩余单字节会以半字形式写入（高位补 0）。
 *        写入过程中每隔 256 个半字会自动喂狗一次（若 FEED_IWDG_AUTO 启用）。
 * 
 * @warning 操作期间会关闭全局中断，完成后恢复。
 */
Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size )
{
  if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || !pData || Buffer_Size == 0 )   
      return CUS_FLASH_PARAMERR;

  if ( (StartAddress & 0x01UL) != 0 )   // 地址未半字对齐. 
  {
    return CUS_FLASH_PARAMERR;
  }

  uint8_t is_NeedExtraEdit = 0;
  if ( (Buffer_Size % 2) != 0 )  // 需要写入的数据为奇数(非2的偶数倍). 最后多一个字节需要特殊处理.   
  {
    is_NeedExtraEdit = 1;
  }

  __disable_irq();
  Cus_Flash_Unlock();

  uint32_t addr = StartAddress;
  uint8_t *pSrc = pData;
  uint32_t size = Buffer_Size;
  uint32_t feed_counter_halfWord = 0;

  while( size >= 2 )
  {
    if ( HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, addr, *(uint16_t *)pSrc) != HAL_OK )
    {
      __enable_irq();
      Cus_Flash_Lock();
      return CUS_FLASH_ERROR;
    }

    addr += 2;
    pSrc += 2;
    size -= 2;
    feed_counter_halfWord++;

    if ( feed_counter_halfWord % 256 == 0 )     // 每写入 256 个半字(512字节)，喂狗一次.
    {
      Cus_Flash_FeedIWDG();
    }
  }

  if ( is_NeedExtraEdit )
  {
    // 还剩最后两字节需要处理.
    uint16_t last_16bit = 0;
    last_16bit = *pSrc; 
    if ( HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, addr, last_16bit) != HAL_OK )
    {
      __enable_irq();
      Cus_Flash_Lock();
      return CUS_FLASH_ERROR;
    }
  }

  __enable_irq();
  Cus_Flash_Lock();
  return CUS_FLASH_OK;
}
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief 校验 Flash 指定区域的数据与缓冲区是否一致
 * @param StartAddress Flash 起始地址
 * @param pData        期望数据的缓冲区指针
 * @param Size         校验长度（字节）
 * @return true 数据完全一致，false 存在不一致
 * 
 * @note  当发现不一致时，会触发 Cus_FLASH_VerifyBufferFailed_Hook 通知调用方。
 *        可用于写入后验证或固件完整性检查。
 */
bool Cus_Flash_VerifyBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Size )
{
  if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || !pData || Size == 0 )  return false;

  for( uint32_t i = 0; i < Size; i++ )
  {
    if ( *((uint8_t *)StartAddress + i) != pData[i] )   
    {
      Cus_FLASH_VerifyBufferFailed_Hook(StartAddress, pData, Size);

      return false;
    }
  }

  return true;
}
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  检查指定 Flash 区域是否处于擦除状态（全 0xFF）
 * @param  StartAddress 起始地址
 * @param  Size         检查长度（字节）
 * @return true 已擦除，false 未擦除或参数非法
 * 
 * @note   常用于上电时判断某区域是否为空，或验证擦除操作是否成功。
 */
bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size )
{
  if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || Size == 0 )  return false;

  for( uint32_t i = 0; i < Size; i++ )
  {
    if ( *((uint8_t *)StartAddress + i) != 0xFF )   return false;
  }

  return true;
}
/* -------------------------------------------------------------------------------------------------------- */



#if defined(FLASH_TYPEERASE_PAGES) 

/* -------------------------------------------------------------------------------------------------------- */
  /**
   * @brief 擦除单个 Flash 页
   * @param PageAddress 页起始地址，必须与 FLASH_BYTES_PER_PAGE 对齐
   * @return Cus_Flash_State_t 操作状态
   * 
   * @note  擦除期间会关闭全局中断并循环等待 BSY 标志清除。
   *        等待过程中每隔约 2000 次指令周期会自动喂狗（若 FEED_IWDG_AUTO 启用）。
   *        擦除完成后会检查 WRPRTERR 和 PGERR 等硬件错误标志。
   * 
   * @warning 擦除操作会将该页所有数据清为 0xFF，请确保页内无有效数据需要保留。
   */
  Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress)
  {
    if ( (PageAddress % FLASH_BYTES_PER_PAGE) != 0)   return CUS_FLASH_PARAMERR;     // 非页对齐.

    if ( (PageAddress < FLASH_BASE) || (PageAddress >= FLASH_END_ADDR) )  return CUS_FLASH_PARAMERR;

    if ( (FLASH->SR & FLASH_SR_BSY_Msk) != 0 )  return CUS_FLASH_BUSY;  // 已有闪存操作正在进行. 返回BUSY.
    __disable_irq();                                // 关中断.
    Cus_Flash_Unlock();

    FLASH->CR = ((FLASH->CR) | (0x01UL << FLASH_CR_PER_Pos));       // 设置 PER 位为1.
    FLASH->AR = (uint32_t)PageAddress;                     // 设置要擦除的页.
    FLASH->CR = ((FLASH->CR) | (0x01UL << FLASH_CR_STRT_Pos));      // 设置 STRT 位为1.

    uint32_t timeout = Cus_Flash_GetSpinCount(FLASH_ERASE_TIMEOUT_MS);
    uint32_t feed_counter = 0;
    while( (FLASH->SR & FLASH_SR_BSY_Msk) != 0 && timeout-- )
    {
      __NOP();

      if ( (++feed_counter % 2000) == 0 )
      {
        Cus_Flash_FeedIWDG();
      }
    }
    __enable_irq();

    if ( timeout == 0 )
    {
      FLASH->CR &= ~FLASH_CR_PER_Msk;
      Cus_FLASH_EraseFailed_Hook();
      Cus_Flash_Lock();
      return CUS_FLASH_TIMEOUT;
    }

    if ( FLASH->SR & (FLASH_SR_WRPRTERR_Msk | FLASH_SR_PGERR_Msk) )   // 检查错误标志.
    {
      if ( (FLASH->SR & (FLASH_SR_WRPRTERR_Msk | FLASH_SR_PGERR_Msk)) == (FLASH_SR_WRPRTERR_Msk | FLASH_SR_PGERR_Msk) ) // 双重错误.
      {
        Cus_FLASH_EraseFailed_Hook();
        FLASH->SR |= ((0x01UL << FLASH_SR_WRPRTERR_Pos) | (0x01UL << FLASH_SR_PGERR_Pos));
        FLASH->CR &= ~FLASH_CR_PER_Msk;
        Cus_Flash_Lock();
        return CUS_FLASH_ERROR;
      }

      int8_t flag = 0;
      if ( FLASH->SR & FLASH_SR_WRPRTERR_Msk )
      {
        flag = 1;
        Cus_FLASH_EraseFailed_Hook();
      }

      if ( FLASH->SR & FLASH_SR_PGERR_Msk )
      {
        flag = -1;
        Cus_FLASH_EraseFailed_Hook();
      }

      FLASH->SR = (FLASH->SR | (0x01UL << FLASH_SR_WRPRTERR_Pos) | (0x01UL << FLASH_SR_PGERR_Pos));   // 清除错误标志.
      FLASH->CR &= ~FLASH_CR_PER_Msk;
      Cus_Flash_Lock();
      if ( flag > 0 )  return CUS_FLASH_PROGRAM_WRPRTERR;
      else if ( flag < 0 )  return CUS_FLASH_PROGRAM_PGERR;
    }

    FLASH->CR &= ~FLASH_CR_PER_Msk;
    Cus_Flash_Lock();
    return CUS_FLASH_OK;
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  根据页索引计算页起始地址
 * @param  page_index 页索引（0-based）
 * @return 页起始地址，若索引越界则返回 0
 */
  uint32_t Cus_Flash_GetPageAddress( uint32_t page_index )
  {
    if ( (FLASH_BASE + page_index * FLASH_BYTES_PER_PAGE) >= FLASH_END_ADDR )   return 0;

    return (FLASH_BASE + ( page_index * FLASH_BYTES_PER_PAGE ));
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  获取指定地址所在页的起始地址（向上取整）
 * @param  Address 任意 Flash 地址
 * @return 所在页的起始地址，若地址非法则返回 0；若已是页起始则返回自身
 */
  uint32_t Cus_Flash_GetPageStart( uint32_t Address )
  {
    if ( Address < FLASH_BASE || Address > FLASH_END_ADDR )   return 0;
    
    if ( (Address % FLASH_BYTES_PER_PAGE) == 0 )  return Address;   // 已经属于整页地址. 空调用.

    return Cus_Flash_GetPageAddress( ((Address - FLASH_BASE) / FLASH_BYTES_PER_PAGE) );
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief 获取 Flash 页控制块（工厂方法）
 * @param pPageOut 指向 Cus_Flash_Page_t* 的指针，用于接收实例
 * @return Cus_Flash_State_t 操作状态
 * 
 * @note  根据 CUS_FLASH_USE_DYNAMIC_PAGE 宏选择动态或静态分配。
 *        动态模式：通过 malloc 分配，需调用 Release 释放内存。
 *        静态模式：使用模块内部唯一的静态实例，全局仅允许同时存在一个申请。
 *        无论哪种模式，获取到的结构体均已清零。
 * 
 * @warning 静态模式下该函数不可重入，重复调用会返回 CUS_FLASH_ERROR。
 */
  Cus_Flash_State_t Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut )
  {
    /* 关于Factory_GetPageControlBlock:
            该API用于向用户提供一个动态/静态分配的 Cus_Flash_Page_t 结构，用于后续的整页写入等操作. 由于 Cus_Flash_Page_t 所占空间较大，建议全局仅分配一个 Cus_Flash_Page_t 
            并且当使用完一次后，手动调用成员 Reset(). 便于下次使用.*/

    if ( !pPageOut )    return CUS_FLASH_ERROR;
#if (CUS_FLASH_USE_DYNAMIC_PAGE)
  #warning "If malloc failed. Please check your heap size config."
      Cus_Flash_Page_t *pReturn = (Cus_Flash_Page_t *)malloc(sizeof(Cus_Flash_Page_t));
      if ( !pReturn )
      {
        Cus_FLASH_PageStructMallocFailed_Hook( pPageOut );

        return CUS_FLASH_ERROR;
      }
      *pPageOut = pReturn;
#else 
    static Cus_Flash_Page_t g_PageInstance = { 0 };
    if ( g_is_PageAllocated )   return CUS_FLASH_ERROR;     // 由于Page结构体占用内存空间较大. 因此RAM中只允许分配一份.
    g_is_PageAllocated = true;      // Tips: 该API在 Static 分配模式下是不可重入的!违反调用将返回 CUS_FLASH_ERROR.
    *pPageOut = &g_PageInstance;
#endif // CUS_FLASH_USE_DYNAMIC_PAGE
    
    memset((*pPageOut)->PageDataBuffer, 0, FLASH_BYTES_PER_PAGE);
    (*pPageOut)->PageAddress = 0;
    (*pPageOut)->Reset = PageStructure_Reset;
    (*pPageOut)->Release = PageStructure_Release;

    return CUS_FLASH_OK;
  }
/* -------------------------------------------------------------------------------------------------------- */



/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  写入一页数据到 Flash（先擦除再写入再校验）
 * @param  pPage 页控制块指针，需已填充 PageAddress 和 PageDataBuffer
 * @return Cus_Flash_State_t 操作状态
 * 
 * @note   该函数依次执行：页擦除 -> 缓冲区写入 -> 写入后校验。
 *        任一环节失败均返回对应错误状态。
 * 
 * @warning 调用前必须确保 pPage 由 Factory_GetPageControlBlock 创建。
 */
  Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_Page_t *pPage )
  {
    if ( !pPage || pPage->PageAddress < FLASH_BASE || pPage->PageAddress > FLASH_END_ADDR || ((pPage->PageAddress) % FLASH_BYTES_PER_PAGE) != 0 )
        return CUS_FLASH_PARAMERR;

    if ( Cus_Flash_ErasePage( pPage->PageAddress ) != CUS_FLASH_OK )
    {
      return CUS_FLASH_ERROR;
    }

    if ( Cus_Flash_WriteBuffer(pPage->PageAddress, pPage->PageDataBuffer, FLASH_BYTES_PER_PAGE) != CUS_FLASH_OK )
    {
      Cus_FLASH_PageWriteFailed_Hook( pPage );

      return CUS_FLASH_ERROR;
    }

    if ( !Cus_Flash_VerifyBuffer(pPage->PageAddress, pPage->PageDataBuffer, FLASH_BYTES_PER_PAGE) )
    {
      return CUS_FLASH_ERROR;
    }

    return CUS_FLASH_OK;
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  重置页控制块（清空地址和数据缓存）
 * @param  pPage 待重置的控制块
 * @note   用于复用同一控制块进行多次写入操作，避免重复申请。
 */
  static void PageStructure_Reset( Cus_Flash_Page_t *pPage )
  {
    if ( !pPage )   return;

    pPage->PageAddress = 0;
    memset(pPage->PageDataBuffer, 0, FLASH_BYTES_PER_PAGE);
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  释放页控制块
 * @param  pPage 待释放的控制块
 * @note   动态模式：释放 malloc 的内存；静态模式：标记为未分配并清空。
 *        释放后该指针不应再被使用。
 */
  static void PageStructure_Release( Cus_Flash_Page_t *pPage )
  {
    /* 注意！ 传入的Cus_Flash_Page_t *pPage 必须是经由 Factory_GetPageControlBlock 创建并初始化的.否则将会出现释放错误. */
    if ( !pPage )   return;

    #if (CUS_FLASH_USE_DYNAMIC_PAGE)
      memset(pPage, 0, sizeof(Cus_Flash_Page_t));
      free(pPage);
    #else 
      g_is_PageAllocated = false;
      memset(pPage->PageDataBuffer, 0, FLASH_BYTES_PER_PAGE);
      pPage->PageAddress = 0;
    #endif // CUS_FLASH_USE_DYNAMIC_PAGE

  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  从指定页读取数据到用户缓冲区
 * @param  PageAddress 页起始地址
 * @param  pOutBuffer  输出缓冲区指针
 * @param  Size        期望读取的长度（字节）
 * @return true 读取成功，false 参数非法
 * 
 * @note   若 Size 超过一页大小，仅读取一页数据，多余部分以 0 填充。
 *        若 Size 小于一页，则只读取 Size 个字节。
 */
  bool Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size )
  {
    if ( PageAddress < FLASH_BASE || PageAddress > FLASH_END_ADDR || !pOutBuffer || Size == 0 )  return false;

    uint32_t Read_count = 0;
    if ( Size <= FLASH_BYTES_PER_PAGE )  
        Read_count = Size;
    else  
        Read_count = FLASH_BYTES_PER_PAGE;


    for( uint32_t j = 0; j < Read_count; j++ )
    {
      pOutBuffer[j] = *((uint8_t *)PageAddress + j);
    }

    int32_t remaining = Size - FLASH_BYTES_PER_PAGE;   // 缓冲区给大了，已经读完一页了，剩下空间以0填充.
    if (remaining > 0) 
    {
      memset(&pOutBuffer[FLASH_BYTES_PER_PAGE], 0, remaining);
    }

    return true;
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  获取指定地址所在的页索引
 * @param  Address Flash 地址
 * @return 页索引（0-based），非法地址返回 -1
 */
  int16_t Cus_Flash_GetPageIndex( uint32_t Address )
  {
    if ( Address < FLASH_BASE || Address > FLASH_END_ADDR )   return -1;

    return ((Address - FLASH_BASE) / FLASH_BYTES_PER_PAGE);
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  获取 Flash 总页数
 * @return 页总数
 */
  uint16_t Cus_Flash_GetTotalPages( void )
  {
    return ( (FLASH_END_ADDR - FLASH_BASE) / FLASH_BYTES_PER_PAGE );
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  获取从指定页地址到 Flash 末尾的剩余页数
 * @param  PageAddress 页起始地址（必须页对齐）
 * @return 剩余页数，参数非法返回 -1
 */
  int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress )
  {
    if ( PageAddress < FLASH_BASE || PageAddress > FLASH_END_ADDR || (PageAddress % FLASH_BYTES_PER_PAGE) != 0 )   
          return -1;

    return ( (FLASH_END_ADDR - PageAddress) / FLASH_BYTES_PER_PAGE );
  }
/* -------------------------------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------------------------------- */
/**
 * @brief  擦除多个连续 Flash 页
 * @param  PageStartAddress 起始页地址（必须页对齐）
 * @param  PageCount        待擦除页数
 * @return 成功擦除的页数（等于 PageCount 表示全部成功）
 * 
 * @note   内部逐页调用 Cus_Flash_ErasePage，每擦完一页喂狗一次。
 *        若某页擦除失败则立即返回已成功擦除的页数，不再继续。
 */
  uint16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount )
  {
    if ( PageStartAddress < FLASH_BASE || PageStartAddress > FLASH_END_ADDR || (PageStartAddress % FLASH_BYTES_PER_PAGE) != 0 )   return 0;

    uint16_t PagesRemain = Cus_Flash_GetRemainPages(PageStartAddress);
    uint16_t PagesWaitToBeErase = PageCount;

    if ( PagesWaitToBeErase > PagesRemain )   return 0;

    int16_t CurrectPage = Cus_Flash_GetPageIndex(PageStartAddress);
    uint16_t SuccessFulEraseCount = 0;

    for( uint16_t i = 0; i < PagesWaitToBeErase; i++ )
    {
      if ( Cus_Flash_ErasePage(Cus_Flash_GetPageAddress((uint32_t)CurrectPage + i)) != CUS_FLASH_OK )
      {
        return SuccessFulEraseCount;
      }

      Cus_Flash_FeedIWDG();
      SuccessFulEraseCount++;
    }

    return PagesWaitToBeErase;
  }
/* -------------------------------------------------------------------------------------------------------- */

#endif // FLASH_TYPEERASE_PAGES


/**
 * @brief Flash 解锁失败 Hook（弱函数，用户可重写）
 */
__weak void Cus_FLASH_UnlockFailed_Hook( void )
{
  for( ; ; );
}


/**
 * @brief Flash 上锁失败 Hook（弱函数，用户可重写）
 */
__weak void Cus_FLASH_LockFailed_Hook( void )
{
  for( ; ; );
}


/**
 * @brief Flash 擦除失败 Hook（弱函数，用户可重写）
 */
__weak void Cus_FLASH_EraseFailed_Hook( void )
{
  for( ; ; );
}


/**
 * @brief 页控制块 malloc 失败 Hook（弱函数，用户可重写）
 * @note  仅在动态分配模式下触发。
 */
__weak void Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage )
{
  UNUSED(ppPage);
  for( ; ; );
}


/**
 * @brief 页写入失败 Hook（弱函数，用户可重写）
 * @note  在 Cus_Flash_WritePage 中写入缓冲区失败时触发。
 */
__weak void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage )
{
  UNUSED(pPage);
  for( ; ; );
}


/**
 * @brief 缓冲区校验失败 Hook（弱函数，用户可重写）
 * @note  在 Cus_Flash_VerifyBuffer 中发现数据不一致时触发。
 */
__weak  void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize )
{
  UNUSED(pData);
  UNUSED(BufferSize);
  for( ; ; );
}
