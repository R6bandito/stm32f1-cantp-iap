#include "CAN_Cus.h"


/* ------------------------ g_Ver & Static Ver -------------------------------- */
static CAN_HandleTypeDef canHandle[MAX_SUPPORT_CANDEV] = { 0 };
static Cus_CAN_Device_t *CanDevice[MAX_SUPPORT_CANDEV];
static uint8_t s_CanDeviceUsed[MAX_SUPPORT_CANDEV];


#if (USE_DEFAULT_RxFIFO_FULL_HOOK)
  void *g_pBackUP_Array[MAX_FIFO_FULL_COUNT];
#endif // USE_DEFAULT_RxFIFO_FULL_HOOK

#if (USE_SEND_ASYNC) && (!SEND_ASYNC_NodePOLL_DYNAMIC)
  static TxMsgNode_t s_TxNodePoll[TX_NODE_POLL_SIZE];           // 空闲节点内存池.
  static TxMsgNode_t *s_TxNodeFreeStack[TX_NODE_POLL_SIZE];     // 空闲节点指针栈.

  uint8_t TxFreeStackIndex;    // 空闲节点指针栈栈顶索引.
  uint8_t FreeStackCount;       // 剩余空闲节点数目.
  bool is_NodePollInit = false;
#endif // USE_SEND_ASYNC



// Device结构 私有变量.用户不允许直接访问.
typedef struct Cus_Private
{
  // 用户注册的缓冲区信息.
  void *RxBuffer;
  uint32_t buffer_Size; 

  // 环形缓冲区管理变量（由驱动内部使用）.
  volatile uint16_t head;
  volatile uint16_t tail;
  uint16_t max_msgNum;

  Cus_CAN_RxMsg_t *pRing;        // 将 pBuffer 强制转换为 Cus_CAN_RxMsg_t* 后的指针

  #if (USE_SEND_ASYNC)
    TxMsgNode_t *TxHead;         // 发送队列头. 
    TxMsgNode_t *TxTail;         // 发送队列尾.
    TxMsgNode_t* TxCurrent;      // 当前正在发送的节点.
    uint8_t TxBusy;              // 硬件发送忙碌标志.
  #endif 

} Cus_CAN_Priv_t;


#if (!CAN_TCB_ALLOC_DYNAMIC)
  // 静态分配内存池.
  static Cus_CAN_Device_t s_CanDevicePool[MAX_SUPPORT_CANDEV];
  static Cus_CAN_Priv_t s_CanDevicePriPool[MAX_SUPPORT_CANDEV];
#endif // CAN_TCB_ALLOC_DYNAMIC
/* ---------------------------------------------------------------------------- */

#ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
  U8 Cus_CanTP_canSendFunc_Asynchronous( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc );
  U8 Cus_CanTP_canRecvFunc_Asynchronous( Cus_CANTp_Conn_t *pConn, U32 *pcanId, U8 *pData, U8 *pDlc );
#endif // __Cus_CANTP_XzzwY7a9BBCTQ7__


/* ----------------------------------------------------------------- */
uint8_t Factory_CANInitConfig_t( CANInitConfig_t **pOutConfig );
uint8_t Factory_CANFilterConfig_t( CANFilterConfig_t **pOutConfig );
int8_t Factory_CANInitConfig_t_Static( uint8_t *buffer, uint32_t buffer_size );
int8_t Factory_CANFilterConfig_t_Static( uint8_t *buffer, uint32_t buffer_size );

int8_t Cus_CAN_getIndex( CAN_TypeDef *instance );
CAN_HandleTypeDef *Cus_CAN_getHandle( CAN_TypeDef *instance );
HAL_StatusTypeDef Cus_CAN_getRateInfo( CAN_TypeDef *instance, Cus_CAN_RateInfo_t *pOutInfo );
Cus_CAN_Device_t *Cus_CAN_getControlBlock( CAN_TypeDef *instance );

HAL_StatusTypeDef Cus_CAN_Start( CAN_TypeDef *instance );
void Cus_CAN_DeviceClose( Cus_CAN_Device_t **pDev );
void Cus_CAN_RingRecvIT( Cus_CAN_Device_t *pDev, uint32_t FIFO );

void Cus_CAN_Filter_SetStdList32(  CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint16_t id1, uint16_t id2 );
void Cus_CAN_Filter_SetStdList16( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint16_t id1, uint16_t id2, uint16_t id3, uint16_t id4 );
void Cus_CAN_Filter_SetExtList32( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint32_t id1, uint32_t id2 );
void Cus_CAN_Filter_SetStdMask32( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint16_t id1, uint16_t id1_mask );
void Cus_CAN_Filter_SetExtMask32( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint32_t id1, uint32_t id1_mask );
void Cus_CAN_Filter_SetStdMask16( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint16_t id1, uint16_t id1_mask, uint16_t id2, uint16_t id2_mask );

static uint8_t Cus_CAN_registerRXBuffer( Cus_CAN_Device_t *pDev, void *pBuffer, uint32_t size );
static HAL_StatusTypeDef Cus_CAN_Send( Cus_CAN_Device_t *pDev, CAN_TxHeaderTypeDef Txheader, uint8_t *Send_Buf );
static HAL_StatusTypeDef Cus_CAN_Recv( const Cus_CAN_Device_t *pDev, CAN_RxHeaderTypeDef *pHeader, uint8_t *Recv_Buf, uint32_t RxFifo );
static HAL_StatusTypeDef Cus_CAN_Recv_IT( Cus_CAN_Device_t *pDev, CAN_RxHeaderTypeDef *pHeader, uint8_t *Recv_Buf );
static HAL_StatusTypeDef Cus_CAN_EnbIT( Cus_CAN_Device_t *pDev, uint32_t interrupt_mask );
static HAL_StatusTypeDef cus_canInit( const CANInitConfig_t *pConf_Structure );
static bool CheckInterrupt( const Cus_CAN_Device_t *pDev, uint32_t interrupt_mask );
static uint8_t createCANTCB( Cus_CAN_Device_t **pDevice, const CANInitConfig_t *pInit );
static HAL_StatusTypeDef cus_canfilterInit( const CANFilterConfig_t *pConf_Structure, CAN_TypeDef *instance );

static void __release( CANInitConfig_t **pConf );
static void __release_filter( CANFilterConfig_t **pFilter );
static void __canTCB_release( Cus_CAN_Device_t ** pDevice );

static HAL_StatusTypeDef CAN_GetTimingFromBaudrate(Cus_CAN_Baudrate_t Baudrate, uint32_t *prescaler, uint32_t *pbs1, uint32_t *pbs2);

#if (!CAN_TCB_ALLOC_DYNAMIC)
  static void __canTCB_release_Static( Cus_CAN_Device_t ** pDevice );
#endif 

#if (!CAN_CFG_ALLOC_DYNAMIC)
  static void __release_static( CANInitConfig_t **pConf );
  static void __release_filter_static( CANFilterConfig_t **pFilter );
#endif 

#if (USE_SEND_ASYNC)
  static void Cus_CAN_NodePollInit( Cus_CAN_Device_t *pDev );
  static void Cus_CAN_FreeNode( TxMsgNode_t *pNode );
  static HAL_StatusTypeDef Cus_CAN_Send_IT( Cus_CAN_Device_t *pDev, CAN_TxHeaderTypeDef Txheader, uint8_t *Send_Buf );
  void Cus_CAN_ProcessTxQueue( Cus_CAN_Device_t *pDev );
#endif 
/* ----------------------------------------------------------------- */


#if (CAN_CFG_ALLOC_DYNAMIC)
  uint8_t Factory_CANInitConfig_t( CANInitConfig_t **pOutConfig )
  {
    #warning "The param pOutConfig Must be global ver.Otherwise you will lose the memory that created by DynamicAlloc."
    /* 注意: 传入的 pOutConfig 参数必须是对应调用层的全局变量.若为局部变量，则会丢失通过动态分配的内存从而造成内存泄漏！ */

    if ( !pOutConfig )  return 0xFF;

    CANInitConfig_t *pReturn = (CANInitConfig_t *)malloc(sizeof(CANInitConfig_t));
    if ( pReturn == NULL )  return 0xFF;
    *pOutConfig = pReturn;

    memset(*pOutConfig, 0, sizeof(CANInitConfig_t));
    (**pOutConfig).Cus_CAN_Init = cus_canInit;
    (**pOutConfig).Self_Release = __release;
    (**pOutConfig).is_DynamicAlloc = true;
    return 0;
  }


  uint8_t Factory_CANFilterConfig_t( CANFilterConfig_t **pOutConfig )
  {
    if ( !pOutConfig )  return 0xFF;

    /* 注意: 传入的 pOutConfig 参数必须是对应调用层的全局变量.若为局部变量，则会丢失通过动态分配的内存从而造成内存泄漏！ */
    CANFilterConfig_t *pReturn = (CANFilterConfig_t *)malloc(sizeof(CANFilterConfig_t));
    if ( pReturn == NULL )  return 0xFF;
    *pOutConfig = pReturn;

    memset(*pOutConfig, 0, sizeof(CANFilterConfig_t));
    (*pOutConfig)->Cus_CAN_FilterInit = cus_canfilterInit;
    (*pOutConfig)->Self_Release = __release_filter;
    (*pOutConfig)->is_DynamicAlloc = true;
    return 0;
  }

#endif // CAN_CFG_ALLOC_DYNAMIC



#if (!CAN_CFG_ALLOC_DYNAMIC)
  int8_t Factory_CANInitConfig_t_Static( uint8_t *buffer, uint32_t buffer_size )
  {
    if ( !buffer || buffer_size == 0 )    return -1;

    CANInitConfig_t canInitStructure = { 0 };
    canInitStructure.Cus_CAN_Init = cus_canInit;
    canInitStructure.is_DynamicAlloc = false;
    canInitStructure.Self_Release = __release_static;

    if ( buffer_size < sizeof(canInitStructure) + 1 )   return -2;
    memcpy(buffer, &canInitStructure, sizeof(canInitStructure));

    return 0;
  }


  int8_t Factory_CANFilterConfig_t_Static( uint8_t *buffer, uint32_t buffer_size )
  {
    if ( !buffer || buffer_size == 0 )    return -1;

    CANFilterConfig_t canFilterStructure = { 0 };
    canFilterStructure.Cus_CAN_FilterInit = cus_canfilterInit;
    canFilterStructure.Self_Release = __release_filter_static;
    canFilterStructure.is_DynamicAlloc = false;

    if ( buffer_size < sizeof(canFilterStructure) + 1 )    return -2;
    memcpy(buffer, &canFilterStructure, sizeof(canFilterStructure));

    return 0;
  }

  static void __release_static( CANInitConfig_t **pConf ) { UNUSED(pConf); }   // 采用静态分配的内存由用户自行进行管理，此处release仅作占位.

  static void __release_filter_static( CANFilterConfig_t **pFilter ) { UNUSED(pFilter); }


/* 静态分配使用示例：
  * uint8_t init_buf[sizeof(CANInitConfig_t)];
  *
  * if (Factory_CANInitConfig_t_Static(init_buf, sizeof(init_buf)) != 0) 
  * {
  *     // 初始化失败
  * }
  * 
  * CANInitConfig_t *pInit = (CANInitConfig_t*)init_buf;
  * pInit->Instance = CAN1;
  * // ... 修改其他参数 ...
  * pInit->Cus_CAN_Init(pInit);
  * 
  * 过滤器类似:
  * uint8_t filter_buf[sizeof(CANFilterConfig_t)];
  * Factory_CANFilterConfig_t_Static(filter_buf, sizeof(filter_buf));
  * CANFilterConfig_t *pFilter = (CANFilterConfig_t*)filter_buf;
*/

#endif 


/* -------------------------------------- CAN_TCB Relevant ----------------------------------------------------- */
static void __canTCB_release( Cus_CAN_Device_t ** pDevice )
{
  if ( !pDevice || !(*pDevice) )   return;

  Cus_CAN_Priv_t *pPriv = (Cus_CAN_Priv_t *)((*pDevice)->private);
  if ( pPriv )    free(pPriv);      // 先释放动态分配的成员.

  free(*pDevice);

  *pDevice = NULL;
}


int8_t Cus_CAN_getIndex( CAN_TypeDef *instance )
{
  if ( !instance )   return -1;

  if ( instance == CAN1 )   return CAN1_INDEX;

  #if defined(CAN2)
    if ( instance == CAN2 )   return CAN2_INDEX;
  #endif 

  #if defined(CAN3)
    if ( instance == CAN3 )   return CAN3_INDEX;
  #endif 

  return -2;
}


static void __canTCB_release_Static( Cus_CAN_Device_t ** pDevice )  { UNUSED(pDevice); }

#if (CAN_TCB_ALLOC_DYNAMIC)
  static uint8_t createCANTCB( Cus_CAN_Device_t ** pDevice, const CANInitConfig_t *pInit )
  {
    if ( !pInit )   return 0xFF;

    int8_t index = Cus_CAN_getIndex(pInit->Instance);
    if ( index < 0 )  return 0xFF;

    if ( s_CanDeviceUsed[index] )    return 0xFF;    // 避免重复初始化.

    Cus_CAN_Device_t *pTCB = (Cus_CAN_Device_t *)malloc(sizeof(Cus_CAN_Device_t));
    if ( pTCB == NULL )   return 0xFF;
    *pDevice = pTCB;

    Cus_CAN_Priv_t *pPrivate = (Cus_CAN_Priv_t *)malloc(sizeof(Cus_CAN_Priv_t));
    if ( pPrivate == NULL )   return 0xFF;
    
    pPrivate->RxBuffer = NULL;
    pPrivate->head = 0;
    pPrivate->tail = 0;
    pPrivate->max_msgNum = 0;
    pPrivate->buffer_Size = 0;
    pPrivate->pRing = NULL;

    memset(*pDevice, 0, sizeof(Cus_CAN_Device_t));
    (*pDevice)->Instance = pInit->Instance;
    (*pDevice)->canHandle = Cus_CAN_getHandle(pInit->Instance);
    (*pDevice)->Send = Cus_CAN_Send;
    (*pDevice)->Receive = Cus_CAN_Recv;
    (*pDevice)->EnableInterrupt = Cus_CAN_EnbIT;
    (*pDevice)->CheckInterrupt = CheckInterrupt;
    (*pDevice)->registerRxBuffer = Cus_CAN_registerRXBuffer;
    (*pDevice)->private = (void *)pPrivate;
    (*pDevice)->Receive_IT = Cus_CAN_Recv_IT;

    #if (USE_DEFAULT_RxFIFO_FULL_HOOK)
      (*pDevice)->is_RingFIFO_Full = false;
      (*pDevice)->RingFIFOFull_Count = 0;
      (*pDevice)->pBackUPRecv_Buf = g_pBackUP_Array;
    #endif // USE_DEFAULT_RxFIFO_FULL_HOOK

    s_CanDeviceUsed[index] = 1;

    // 控制TCB不允许人为清理. 不注册清理函数.
    return 0;
  }


  static void __release( CANInitConfig_t **pConf )
  {
    if ( !pConf || !(*pConf) ) return;
    if ( (*pConf)->is_DynamicAlloc != true ) return;
    free(*pConf);
    *pConf = NULL;
  }


  static void __release_filter( CANFilterConfig_t **pFilter )
  {
    if ( !pFilter || !(*pFilter) )  return;
    if ( (*pFilter)->is_DynamicAlloc != true )  return;
    free(*pFilter);
    *pFilter = NULL;
  }
  #endif // CAN_TCB_ALLOC_DYNAMIC


#if (!CAN_TCB_ALLOC_DYNAMIC)
  static uint8_t createCANTCB( Cus_CAN_Device_t ** pDevice, const CANInitConfig_t *pInit )
  {
    if ( !pDevice || !pInit )   return 0xFF;

    int8_t index = Cus_CAN_getIndex(pInit->Instance);
    if ( index < 0 )  return 0xFF;

    if ( s_CanDeviceUsed[index] )    return 0xFF;    // 检查该槽位是否已经被初始化过. 避免重复初始化.

    Cus_CAN_Device_t *pTCB = &s_CanDevicePool[index];
    Cus_CAN_Priv_t *pPriv = &s_CanDevicePriPool[index];

    pPriv->RxBuffer = NULL;
    pPriv->head = 0;
    pPriv->tail = 0;
    pPriv->max_msgNum = 0;
    pPriv->pRing = NULL;
    pPriv->buffer_Size = 0;

    pTCB->Instance = pInit->Instance;
    pTCB->canHandle = Cus_CAN_getHandle(pInit->Instance);
    pTCB->Send = Cus_CAN_Send;

    #if (USE_SEND_ASYNC)
      pTCB->Send_IT = Cus_CAN_Send_IT;
    #endif 

    pTCB->Receive = Cus_CAN_Recv;
    pTCB->EnableInterrupt = Cus_CAN_EnbIT;
    pTCB->CheckInterrupt = CheckInterrupt;
    pTCB->registerRxBuffer = Cus_CAN_registerRXBuffer;
    pTCB->private = (void *)pPriv;
    pTCB->Receive_IT = Cus_CAN_Recv_IT;

    #if (USE_DEFAULT_RxFIFO_FULL_HOOK)
      pTCB->is_RingFIFO_Full = false;
      pTCB->RingFIFOFull_Count = 0;
      pTCB->pBackUPRecv_Buf = g_pBackUP_Array;
    #endif // USE_DEFAULT_RxFIFO_FULL_HOOK

    *pDevice = pTCB;
    s_CanDeviceUsed[index] = 1;

    #if (USE_SEND_ASYNC)
      Cus_CAN_NodePollInit(*pDevice);     // 若开启异步发送. 则于此处进行初始化.
    #endif 

    return 0;   // Normal Back.
  }
#endif // CAN_TCB_ALLOC_DYNAMIC


/**
 * @brief 获取指定 CAN 实例的设备控制块指针
 * @param instance CAN 外设实例（CAN1/CAN2/CAN3）
 * @return 指向 Cus_CAN_Device_t 结构体的指针，若实例无效或未初始化则返回 NULL
 *
 * @note 该函数返回的设备控制块包含发送、接收、中断使能等函数指针，
 *       用户可通过该指针调用驱动提供的 API（如 dev->Send、dev->Receive_IT 等）。
 *       驱动内部维护该结构体，用户可读取其中的成员（如 Instance、canHandle），
 *       但不应随意修改，以免破坏驱动状态。
 *       调用前请确保对应的 CAN 实例已通过 Cus_CAN_Init 或快速配置函数初始化。
 *
 * @warning 返回的指针仅当 CAN 已初始化且设备对象有效时才可安全使用。
 *          设备对象在驱动生命周期内保持不变，用户无需释放。
 */
  Cus_CAN_Device_t *Cus_CAN_getControlBlock( CAN_TypeDef *instance )
  {
    if ( !instance )  return NULL;

    int8_t index = Cus_CAN_getIndex(instance);

    if ( index < 0 || index > MAX_SUPPORT_CANDEV )  return NULL;

    if ( !s_CanDeviceUsed[index] )    return NULL;    // 检查静态内存池对应的index是否已被CUS库正确初始化.

    return CanDevice[index];
  }
/* --------------------------------------------------------------------------------------------------- */


void Cus_CAN_DeviceClose( Cus_CAN_Device_t **pDev )
{
  if ( !pDev || !(*pDev) )    return;

  CAN_HandleTypeDef *phcan = Cus_CAN_getHandle((*pDev)->Instance);
  if ( !phcan )   return;

  int8_t index = Cus_CAN_getIndex((*pDev)->Instance);
  if ( index < 0 )   return;

/* -------------- 复位 CAN 外设 ------------------ */
  HAL_CAN_AbortTxRequest(phcan, CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2);   // 中止可能的Tx请求.

  HAL_CAN_Stop(phcan);

  HAL_CAN_DeInit(phcan);
/* ---------------------------------------------- */

  #if (CAN_TCB_ALLOC_DYNAMIC)
    // 动态分配: 释放TCB与私有数据.
    __canTCB_release(pDev);     // 释放后 *pDev 为NULL. 
    CanDevice[index] = NULL;    // pDev也为NULL. 不能够再进行使用！ 除非重新初始化.
  #endif 

  #if (!CAN_TCB_ALLOC_DYNAMIC)
    // 静态分配: 重置状态.
    __canTCB_release_Static(pDev);    // 注意. 该API为空逻辑，此处调用仅为了形式上统一.

    Cus_CAN_Priv_t *pPriv = &s_CanDevicePriPool[index];
    memset(pPriv, 0, sizeof(Cus_CAN_Priv_t));

    Cus_CAN_Device_t *pDevbuffer = &s_CanDevicePool[index];
    memset(pDevbuffer, 0, sizeof(Cus_CAN_Device_t));
  #endif 

  s_CanDeviceUsed[index] = 0;
} 


static HAL_StatusTypeDef CAN_GetTimingFromBaudrate(Cus_CAN_Baudrate_t Baudrate, uint32_t *prescaler, uint32_t *pbs1, uint32_t *pbs2)
{
  if ( !prescaler || !pbs1 || !pbs2 )   return HAL_ERROR;

  uint32_t pclk_clock = HAL_RCC_GetPCLK1Freq();

  uint32_t baudrate;
  switch(Baudrate)
  {
    case CAN_BAUDRATE_125K:  baudrate = (125000UL); break;
    case CAN_BAUDRATE_250K:  baudrate = (250000UL); break;
    case CAN_BAUDRATE_500K:  baudrate = (500000UL); break;
    case CAN_BAUDRATE_1M:    baudrate = (1000000UL); break;
  }

  uint32_t target_bit_time_ns = (1000000000UL) / baudrate;
  uint32_t best_error = UINT32_MAX;
  uint32_t best_prescaler = 0, best_pbs1 = 0, best_pbs2 = 0;

  for( uint16_t prescaler = 1; prescaler < 256; prescaler++ )
  {
    uint32_t can_clock = pclk_clock / prescaler;        // APB BUS分频后，CAN外设的实际工作频率.
    uint32_t tq_ns = (1000000000UL) / can_clock;        // 1个tq的时间 换算为ns 避免浮点运算.

    uint32_t min_tq = 8, max_tq = 25;
    uint32_t tq_num = target_bit_time_ns / tq_ns;
    if ( tq_num < min_tq || tq_num > max_tq )  continue;  // 确保一个位中的tq数符合CAN规范(8~25).

    /* 尝试分配 BS1 和 BS2，使采样点约 75% */
    uint32_t bs1 = ( tq_num *  3 ) / 4 ;
    uint32_t bs2 = tq_num - bs1 - 1;                  // Ps: PTS传播段已被硬件整合进 PBS1 段中.
    if ( bs1 < 1 || bs1 > 16 )   continue;
    if ( bs2 < 1 || bs2 > 8 )   continue;

    uint32_t actual_bit_time_ns = tq_ns * ( 1 + bs1 + bs2 );
    uint32_t error = ( actual_bit_time_ns > target_bit_time_ns ) ? (actual_bit_time_ns - target_bit_time_ns) : (target_bit_time_ns - actual_bit_time_ns);
    if ( error < best_error )
    {
      best_error = error;
      best_prescaler = prescaler;
      best_pbs1 = bs1;
      best_pbs2 = bs2;

      if ( error == 0 )   break;              // 精准匹配. 已是最优解，直接退出循环.
    }
  }

  if ( best_prescaler == 0 )  return HAL_ERROR;

  *prescaler = best_prescaler;
  *pbs1 = best_pbs1;
  *pbs2 = best_pbs2;

  return HAL_OK;
}



static HAL_StatusTypeDef cus_canInit( const CANInitConfig_t * pConf_Structure )
{
  if ( !pConf_Structure )   return HAL_ERROR;

  if ( pConf_Structure->Instance == CAN1 )  __HAL_RCC_CAN1_CLK_ENABLE();
  #if defined(CAN2)
    if ( pConf_Structure->Instance == CAN2 )  __HAL_RCC_CAN2_CLK_ENABLE();
  #endif // CAN2
  #if defined(CAN3)
    if ( pConf_Structure->Instance == CAN3 )  __HAL_RCC_CAN3_CLK_ENABLE();
  #endif // CAN3

  GPIO_TypeDef *port = pConf_Structure->CAN_gpio.CAN_GPIOPort_x;
  if ( port == GPIOA )  __HAL_RCC_GPIOA_CLK_ENABLE();
  else if ( port == GPIOB )  __HAL_RCC_GPIOB_CLK_ENABLE();
  else if ( port == GPIOC )  __HAL_RCC_GPIOC_CLK_ENABLE();
  else if ( port == GPIOD )  __HAL_RCC_GPIOD_CLK_ENABLE();
  /* 可根据需要添加其它端口. */
  /* else if ( port == GPIOE )  __HAL_RCC_GPIOE_CLK_ENABLE(); */ 

  GPIO_InitTypeDef gpio = { 0 };
  gpio.Pin = pConf_Structure->CAN_gpio.CAN_GPIO_TX;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  #if defined(STM32F4xx) || defined(STM32ADVANCE)
    gpio.Alternate = pConf_Structure->CAN_gpio.Alternate;
  #endif // STM32F4xx

  HAL_GPIO_Init(port, &gpio);

  gpio.Pin = pConf_Structure->CAN_gpio.CAN_GPIO_RX;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_NOPULL;
  #if defined(STM32F4xx) || defined(STM32ADVANCE)
    gpio.Alternate = pConf_Structure->CAN_gpio.Alternate;
  #endif // STM32F4xx
  HAL_GPIO_Init(port, &gpio);

  CAN_HandleTypeDef *hcan;
  if ( pConf_Structure->Instance == CAN1 )
  {
    hcan = &canHandle[CAN1_INDEX];
  }
  
  #if defined(CAN2)
    if ( pConf_Structure->Instance == CAN2 )
    {
      hcan = &canHandle[CAN2_INDEX];
    }
  #endif // CAN2

  #if defined(CAN3)
    if ( pConf_Structure->Instance == CAN3 )
    {
      hcan = &canHandle[CAN3_INDEX];
    }
  #endif // CAN3

  hcan->Instance = pConf_Structure->Instance;
  uint32_t prescaler, pbs1, pbs2;
  if ( CAN_GetTimingFromBaudrate(pConf_Structure->baudrate, &prescaler, &pbs1, &pbs2) != HAL_OK )
  {
    return HAL_ERROR;
  }

  prescaler = prescaler;
  pbs1 = ((pbs1 - 1) << 16);
  pbs2 = ((pbs2 - 1) << 20);

  uint32_t SyncJumpWidth = 0;
  switch ( pConf_Structure->SJW )
  {
    case Cus_CAN_SJW_1Tq: SyncJumpWidth = CAN_SJW_1TQ;  break;
    case Cus_CAN_SJW_2Tq: SyncJumpWidth = CAN_SJW_2TQ;  break;
    case Cus_CAN_SJW_3Tq: SyncJumpWidth = CAN_SJW_3TQ;  break;
    case Cus_CAN_SJW_4Tq: SyncJumpWidth = CAN_SJW_4TQ;  break;

    default:  SyncJumpWidth = CAN_SJW_1TQ;
  }

  uint32_t can_Mode = 0;
  switch ( pConf_Structure->Mode )
  {
    case MODE_NORMAL:           can_Mode =  CAN_MODE_NORMAL;   break;
    case MODE_LOOPBACK:         can_Mode =  CAN_MODE_LOOPBACK;   break;
    case MODE_SILENT:           can_Mode =  CAN_MODE_SILENT;   break;
    case MODE_SILENT_LPBACK:    can_Mode =  CAN_MODE_SILENT_LOOPBACK;   break;
  
    default:  can_Mode = CAN_MODE_NORMAL;
  }

  hcan->Init.AutoBusOff = pConf_Structure->is_AutoBusOff ? ENABLE : DISABLE;
  hcan->Init.AutoRetransmission = pConf_Structure->is_AutoRestransmission ? ENABLE : DISABLE;
  hcan->Init.AutoWakeUp = pConf_Structure->is_AutoWakeUP ? ENABLE : DISABLE;
  hcan->Init.ReceiveFifoLocked = pConf_Structure->is_ReceiveFifoLocked ? ENABLE : DISABLE;
  hcan->Init.TimeTriggeredMode = pConf_Structure->is_TimeTriggeredMode ? ENABLE : DISABLE;
  hcan->Init.TransmitFifoPriority = pConf_Structure->is_TransmitFifoPriority ? ENABLE : DISABLE;
  hcan->Init.Prescaler = prescaler;
  hcan->Init.TimeSeg1 = pbs1;
  hcan->Init.TimeSeg2 = pbs2;
  hcan->Init.SyncJumpWidth = SyncJumpWidth;
  hcan->Init.Mode = can_Mode;

  HAL_StatusTypeDef hReturn = HAL_CAN_Init(hcan);
  if ( hReturn != HAL_OK )
  {
    Cus_CANInitFailed_Hook(hcan, (const CANInitConfig_t *)pConf_Structure, hReturn);

    return HAL_ERROR;
  }    

  if ( pConf_Structure->Instance == CAN1 ) 
  {
    if ( createCANTCB( &CanDevice[CAN1_INDEX], pConf_Structure ) != 0 )    return HAL_ERROR;
  }
  #if defined(CAN2)
    if ( pConf_Structure->Instance == CAN2 )
    {
      if ( createCANTCB( &CanDevice[CAN2_INDEX], pConf_Structure ) != 0 )    return HAL_ERROR;
    }
  #endif // CAN2
  #if defined(CAN3)
    if ( pConf_Structure->Instance == CAN3 )
    {
      if ( createCANTCB( &CanDevice[CAN3_INDEX], pConf_Structure ) != 0 )    return HAL_ERROR;
    }
  #endif // CAN3

  return HAL_OK; 
}



/**
 * @brief 使能 CAN 控制器中断（同时自动配置 NVIC）
 * @param pDev           指向 CAN 设备对象的指针
 * @param interrupt_mask HAL 库定义的中断掩码，例如：
 *                        - CAN_IT_RX_FIFO0_MSG_PENDING
 *                        - CAN_IT_RX_FIFO1_MSG_PENDING
 *                        - CAN_IT_TX_MAILBOX_EMPTY
 *                        - CAN_IT_ERROR
 *                        - 可按位或组合多个中断源
 * @return HAL_OK 成功；HAL_ERROR 参数错误或 CAN 未启动；HAL_TIMEOUT 启动超时
 *
 * @note 该函数会检查 CAN 是否已启动（INAK 位），若未启动则自动调用 Cus_CAN_Start()。
 *       使能中断后会自动调用 Cus_CAN_NVIC_Config() 配置 NVIC 优先级。
 *       使用前请确保已调用 Cus_CAN_Start() 或驱动已初始化。
 *       用户应使用此函数而非直接调用 HAL_CAN_ActivateNotification()，以保持 NVIC 同步。
 *
 * @warning 如果同时使能 FIFO0 和 FIFO1 的中断，必须确保在 Cus_CAN_NVIC_Config 中
 *          为两者设置相同的优先级（默认均为 5），否则可能破坏环形缓冲区。
 */
static HAL_StatusTypeDef Cus_CAN_EnbIT( Cus_CAN_Device_t *pDev, uint32_t interrupt_mask )
{
  if ( !pDev || !pDev->canHandle )    return HAL_ERROR;

  uint8_t check_INAK = (uint8_t)((uint32_t)(pDev->Instance->MSR) & 0x01UL );
  if ( check_INAK != 0x00 )     // 检查CAN是否已启动. 只有当CAN处于运行状态时才能Activate中断.
  {
    HAL_StatusTypeDef hReturn = Cus_CAN_Start(pDev->Instance);
    if ( hReturn != HAL_OK )
    {
      Cus_CANStartFailed_Hook(pDev->canHandle, hReturn);

      return HAL_TIMEOUT;
    }
  }
  HAL_StatusTypeDef hReturn = HAL_CAN_ActivateNotification(pDev->canHandle, interrupt_mask);
  if ( hReturn != HAL_OK )    return HAL_ERROR;

  if ( !pDev->CheckInterrupt(pDev, interrupt_mask) )   return HAL_ERROR;                // 额外检查. 
  Cus_CAN_NVIC_Config(pDev);                                                            // 更新NVIC配置.

  return HAL_OK;
}


/**
 * @brief 检查 CAN 控制器是否已使能指定的中断
 * @param pDev           指向 CAN 设备对象的指针
 * @param interrupt_mask HAL 库定义的中断掩码（同 Cus_CAN_EnbIT 的 mask 参数）
 * @return true  指定的中断已使能（IER 寄存器中对应位为 1）
 * @return false 指定的中断未使能或参数错误
 *
 * @note 该函数直接读取 CAN 控制器的 IER 寄存器，查询中断使能状态。
 *       可用于判断是否已通过 Cus_CAN_EnbIT 开启了某个中断。
 *       通常用于避免轮询模式与中断模式冲突（例如在 Cus_CAN_Recv 中检查中断是否已使能）。
 *
 * @warning 该函数不检查 NVIC 层中断是否开启，仅反映 CAN 控制器内部中断使能状态。
 */
static bool CheckInterrupt( const Cus_CAN_Device_t *pDev, uint32_t interrupt_mask )
{
  if ( !pDev || !pDev->canHandle )  return false;

  uint32_t check_IT = ((pDev->Instance->IER) & interrupt_mask);
  if ( check_IT == interrupt_mask )   return true;
  else return false;
}


/* -------------------------------------- CAN_get API Relevant ----------------------------------------------------- */

/**
 * @brief 获取指定 CAN 实例的 HAL 句柄指针
 * @param instance CAN 外设实例（CAN1/CAN2/CAN3）
 * @return 指向 CAN_HandleTypeDef 的指针，若实例无效或超出范围则返回 NULL
 *
 * @note 该函数用于内部驱动获取 HAL 句柄，也可供用户在需要直接调用 HAL 库函数时使用。
 *       返回的句柄由驱动在初始化时创建并维护，用户不应修改其内容。
 *       调用前需确保 CAN 已初始化（否则可能返回 NULL）。
 *
 * @warning **返回的句柄指针仅当对应的 CAN 实例已通过 Cus_CAN_Init 或快速配置函数初始化后才有效**。
 */
CAN_HandleTypeDef *Cus_CAN_getHandle( CAN_TypeDef *instance )
{
  if ( !instance )   return NULL;
  if ( instance == CAN1 )   return &canHandle[0];

  #if defined(CAN2)
    if ( instance == CAN2 )   return &canHandle[1];
  #endif // CAN2

  #if defined(CAN3)
    if ( instance == CAN3 )   return &canHandle[2];
  #endif // CAN3

  return NULL;
}


/**
 * @brief 获取 CAN 当前通信速率信息
 * @param instance CAN 外设实例（CAN1/CAN2/CAN3）
 * @param pOutInfo 指向速率信息结构体的指针，用于存储结果
 * @return HAL_OK 成功；HAL_ERROR 参数无效或读取失败
 *
 * @note 该函数通过读取 CAN 控制器的 BTR 寄存器获取当前配置的预分频器、BS1、BS2，
 *       并根据 APB1 时钟频率（HAL_RCC_GetPCLK1Freq()）计算实际波特率。
 *       返回的波特率为浮点数，可能因整数运算存在微小舍入误差。
 *       调用前需确保 CAN 已初始化（否则 BTR 寄存器可能为默认值，导致信息无效）。
 *
 * @warning 该函数假设 CAN 时钟来自 APB1 总线（适用于 STM32F1 系列）。若芯片 CAN 时钟源不同，
 *          请自行修改内部时钟获取函数或覆盖此函数。
 */
HAL_StatusTypeDef Cus_CAN_getRateInfo( CAN_TypeDef *instance, Cus_CAN_RateInfo_t *pOutInfo )
{
  if ( !instance || !pOutInfo )   return HAL_ERROR;

  CAN_HandleTypeDef *hcan = Cus_CAN_getHandle(instance);
  if ( hcan == NULL )   return HAL_ERROR;

  uint32_t BTR_cpy = (uint32_t)hcan->Instance->BTR;
  uint32_t pres = ( BTR_cpy & 0x3FF ) + 1;

  uint32_t temp_bs1 = (( BTR_cpy >> 16 ) & 0xF) + 1;
  uint32_t temp_bs2 = (( BTR_cpy >> 20 ) & 0x7) + 1;
  // 有效性检查（根据 STM32 参考手册）
  if (pres < 1 || pres > 1024 ||
    temp_bs1 < 1 || temp_bs1 > 16 ||
    temp_bs2 < 1 || temp_bs2 > 8) 
  {
    return HAL_ERROR;
  }

  pOutInfo->prescaler = pres;
  pOutInfo->bs1 = temp_bs1;
  pOutInfo->bs2 = temp_bs2;

  pOutInfo->can_clock = HAL_RCC_GetPCLK1Freq();

  float tq = (float)pOutInfo->prescaler / (float)pOutInfo->can_clock;
  float bit_time = (float)(1 + pOutInfo->bs1 + pOutInfo->bs2) * tq;
  pOutInfo->real_baudrate = 1.0f / bit_time;

  return HAL_OK;
}
/* -------------------------------------- CAN_get API Relevant ----------------------------------------------------- */




static HAL_StatusTypeDef cus_canfilterInit( const CANFilterConfig_t * pConf_Structure, CAN_TypeDef *instance )
{
  if ( !pConf_Structure || !instance )   return HAL_ERROR;

  CAN_FilterTypeDef temp_filter = { 0 };
  CAN_HandleTypeDef *hcan;
  hcan = Cus_CAN_getHandle(instance);

  uint32_t filterMode = 0;
  switch(pConf_Structure->Mode)
  {
    case Cus_CAN_FILTERMODE_IDMASK: filterMode = CAN_FILTERMODE_IDMASK; break;
    case Cus_CAN_FILTERMODE_IDLIST: filterMode = CAN_FILTERMODE_IDLIST; break;
    default: filterMode = CAN_FILTERMODE_IDLIST;
  }

  uint32_t fifo_assignment = 0;
  switch (pConf_Structure->FIFOAssignment)
  {
    case Cus_CAN_FIFOASSIGNMENT_FIFO0:  fifo_assignment = CAN_FILTER_FIFO0; break;
    case Cus_CAN_FIFOASSIGNMENT_FIFO1:  fifo_assignment = CAN_FILTER_FIFO1; break;
    default: fifo_assignment = CAN_FILTER_FIFO0; 
  }

  uint32_t filter_scale = 0;
  switch (pConf_Structure->Scale)
  {
    case Cus_CAN_SCALE_16BIT: filter_scale = CAN_FILTERSCALE_16BIT; break;
    case Cus_CAN_SCALE_32BIT: filter_scale = CAN_FILTERSCALE_32BIT; break;
    default:  filter_scale = CAN_FILTERSCALE_16BIT;
  }

  uint32_t filter_activation = 0;
  switch (pConf_Structure->is_Activation)
  {
    case Cus_FILTER_Enable: filter_activation = CAN_FILTER_ENABLE; break;
    case Cus_FILTER_Disable: filter_activation = CAN_FILTER_DISABLE; break;
    default:  filter_scale = CAN_FILTERSCALE_16BIT;
  }  

  temp_filter.FilterFIFOAssignment = fifo_assignment;
  temp_filter.FilterMode = filterMode;
  temp_filter.FilterBank = pConf_Structure->FilterBank;
  temp_filter.FilterScale = filter_scale;

  /* 滤波器ID配置部分将 直接使用由用户填充好的 ID 和掩码 */
  /* 所以调用该方法时, 请确保上层已经正确填充 Id */
  temp_filter.FilterIdHigh = pConf_Structure->IdHigh;
  temp_filter.FilterIdLow = pConf_Structure->IdLow;
  temp_filter.FilterMaskIdHigh = pConf_Structure->MaskIdHigh;
  temp_filter.FilterMaskIdLow = pConf_Structure->MaskIdLow;

  temp_filter.FilterActivation = filter_activation;

  HAL_StatusTypeDef hReturn = HAL_CAN_ConfigFilter(hcan, (const CAN_FilterTypeDef *)&temp_filter);
  if ( hReturn != HAL_OK )
  {
    Cus_FilterConfigFailed_Hook( hcan, (const CANFilterConfig_t *)pConf_Structure, hReturn );

    return HAL_ERROR;
  }

  return HAL_OK; 
}


/**
 * @brief 启动指定 CAN 外设（使能正常通信）
 * @param instance CAN 外设实例（CAN1/CAN2/CAN3）
 * @return HAL_OK 成功；HAL_ERROR 实例无效或启动失败
 *
 * @note 该函数调用 HAL_CAN_Start 启动 CAN 外设，使其进入正常工作模式。
 *       启动后，CAN 控制器才能发送和接收消息。
 *       若启动失败，会调用 Cus_CANStartFailed_Hook 弱函数，用户可覆盖以自定义错误处理。
 *       该函数通常在配置完过滤器后调用，且仅在 CAN 初始化完成且未启动时使用。
 *       重复调用不会产生副作用，但建议避免。
 *
 * @warning 启动前请确保 CAN 已通过 Cus_CAN_Init 初始化，且时钟和 GPIO 已正确配置。
 *          若使能了接收中断，需在启动后再使能中断（或先启动后使能）。
 */
HAL_StatusTypeDef Cus_CAN_Start( CAN_TypeDef *instance )
{
  if ( !instance )  return HAL_ERROR;

  CAN_HandleTypeDef *pcan = Cus_CAN_getHandle(instance);
  if ( pcan == NULL )   return HAL_ERROR;

  HAL_StatusTypeDef hReturn = HAL_CAN_Start( pcan );
  if ( hReturn != HAL_OK )
  { 
    Cus_CANStartFailed_Hook(pcan, hReturn);

    return HAL_ERROR;
  }

  return HAL_OK;
}



/* ------------------------------------------------------------------------- */
static HAL_StatusTypeDef Cus_CAN_Send( Cus_CAN_Device_t *pDev, CAN_TxHeaderTypeDef Txheader, uint8_t *Send_Buf )
{
  if ( !pDev || !Send_Buf || !pDev->canHandle || !pDev->Instance )    return HAL_ERROR;

  if ( Txheader.DLC > 8 )   Txheader.DLC = 8;       // 长度限制为8.

  uint32_t TxMailBox;
  HAL_StatusTypeDef hReturn = HAL_CAN_AddTxMessage(pDev->canHandle, &Txheader, Send_Buf, &TxMailBox);
  if ( hReturn != HAL_OK )
  {
    return HAL_ERROR;
  }

  uint32_t start_tick = HAL_GetTick();  
  uint32_t txok_mask;

  switch (TxMailBox)
  {
    case 1: txok_mask = CAN_TSR_TXOK0; break;
    case 2: txok_mask = CAN_TSR_TXOK1; break;
    case 4: txok_mask = CAN_TSR_TXOK2; break;
    default: return HAL_ERROR;
  }

  // 等待发送完成标志位置起.
  while( (pDev->canHandle->Instance->TSR & txok_mask) == 0 )
  {
    if ( (HAL_GetTick() - start_tick) >= 100 )   // 100ms 超时.
    {
			Cus_CANSendFailed_Hook(pDev, hReturn);

      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}


static HAL_StatusTypeDef Cus_CAN_Recv( const Cus_CAN_Device_t *pDev, CAN_RxHeaderTypeDef *pHeader, uint8_t *Recv_Buf, uint32_t RxFifo )
{
  if ( !pDev || !pHeader || !Recv_Buf )   return HAL_ERROR;

  if ( RxFifo != CAN_RX_FIFO0 && RxFifo != CAN_RX_FIFO1 )   return HAL_ERROR;

  bool is_RxITEnb = pDev->CheckInterrupt( pDev, CAN_IT_RX_FIFO0_MSG_PENDING );
  if ( is_RxITEnb )   return HAL_BUSY;    // 检查中断. 若接收中断开启，则不允许再调用该API.

  return HAL_CAN_GetRxMessage(pDev->canHandle, RxFifo, pHeader, Recv_Buf);
}


static HAL_StatusTypeDef Cus_CAN_Recv_IT( Cus_CAN_Device_t *pDev, CAN_RxHeaderTypeDef *pHeader, uint8_t *Recv_Buf )
{
  if ( !pDev || !pDev->canHandle || !pHeader || !Recv_Buf )   return HAL_ERROR;

  bool is_RxFifo0IT_Enb = pDev->CheckInterrupt(pDev, CAN_IT_RX_FIFO0_MSG_PENDING);
  bool is_RxFifo1IT_Enb = pDev->CheckInterrupt(pDev, CAN_IT_RX_FIFO1_MSG_PENDING);

  if ( !is_RxFifo0IT_Enb && !is_RxFifo1IT_Enb )
  {
    // FIFO1接收中断 与 FIFO2 接收中断均未开启. 空调用.
    return HAL_ERROR;
  }

  Cus_CAN_Priv_t *pPrivate = (Cus_CAN_Priv_t *)pDev->private;
  if ( !pPrivate->RxBuffer || !pPrivate->pRing )
  {
    // 缓冲区未注册. 空调用.
    return HAL_ERROR;
  }
  if ( pPrivate->head == pPrivate->tail )
  {
    // 缓冲区为空. 无可接收数据.
    return HAL_ERROR;
  }

  Cus_CAN_RxMsg_t *pMsg = &pPrivate->pRing[pPrivate->tail];
  *pHeader = pMsg->RxHeader;
  memcpy(Recv_Buf, pMsg->RxData, pMsg->RxHeader.DLC);
  pPrivate->tail = (pPrivate->tail + 1) % pPrivate->max_msgNum;

  return HAL_OK;
}

/* ------------------------------------------------------------------------- */




void Cus_CAN_Filter_SetStdList32(  CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint16_t id1, uint16_t id2 )
{
  if ( id1 > 0x7FF || id2 > 0x7FF || !pFilter )  return;

  uint32_t ide_bit = 0;

  uint8_t RTR_bit_1 = (Filter_RTR & 0x01UL);
  RTR_bit_1 = (RTR_bit_1 >> 0) & 0x01UL;
  uint32_t id1_regVal = ((uint32_t)id1 << 21) | (RTR_bit_1 << 1) | (ide_bit << 2);

  uint8_t RTR_bit_2 = (Filter_RTR & (0x01UL << 1));
  RTR_bit_2 = (RTR_bit_2 >> 1) & 0x01UL;
  uint32_t id2_regVal = ((uint32_t)id2 << 21) | (RTR_bit_2 << 1) | (ide_bit << 2);

  pFilter->IdHigh = (id1_regVal >> 16) & 0xFFFF;      // 高16位放在IDHigh.
  pFilter->IdLow = id1_regVal & 0xFFFF;               // 低16位放在IDLow.
  pFilter->MaskIdHigh = (id2_regVal >> 16) & 0xFFFF;
  pFilter->MaskIdLow = id2_regVal & 0xFFFF;
}


void Cus_CAN_Filter_SetStdList16( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint16_t id1, uint16_t id2, uint16_t id3, uint16_t id4 )
{
  if ( !pFilter || id1 > 0x7FF || id2 > 0x7FF || id3 > 0x7FF || id4 > 0x7FF )   return;

  uint32_t ide_bit = 0;
  uint8_t RTR_bit_1 = (Filter_RTR & 0x01UL);    // 取第一位.
  RTR_bit_1 = RTR_bit_1 >> 0 & 0x01UL;
  uint32_t id1_regVal = ((uint32_t)id1 << 5 ) | (RTR_bit_1 << 4) | (ide_bit << 3);

  uint8_t RTR_bit_2 = (Filter_RTR & (0x01UL << 1)); 
  RTR_bit_2 = RTR_bit_2 >> 1 & 0x01UL;
  uint32_t id2_regVal = ((uint32_t)id2 << 5 ) | (RTR_bit_2 << 4) | (ide_bit << 3);

  uint8_t RTR_bit_3 = (Filter_RTR & (0x01UL << 2));
  RTR_bit_3 = RTR_bit_3 >> 2 & 0x01UL;
  uint32_t id3_regVal = ((uint32_t)id3 << 5 ) | (RTR_bit_3 << 4) | (ide_bit << 3);

  uint8_t RTR_bit_4 = (Filter_RTR & (0x01UL << 3));
  RTR_bit_4 = RTR_bit_4 >> 3 & 0x01UL;
  uint32_t id4_regVal = ((uint32_t)id4 << 5 ) | (RTR_bit_4 << 4) | (ide_bit << 3);

  pFilter->IdHigh = id1_regVal;
  pFilter->IdLow = id2_regVal;
  pFilter->MaskIdHigh = id3_regVal;
  pFilter->MaskIdLow = id4_regVal;
}


void Cus_CAN_Filter_SetExtList32( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint32_t id1, uint32_t id2 )
{
  if ( !pFilter || id1 > 0x1FFFFFFF || id2 > 0x1FFFFFFF )  return;

  uint32_t ide = 1;
  uint8_t RTR_bit_1 = (Filter_RTR & 0x01UL);
  RTR_bit_1 = (RTR_bit_1 >> 0) & 0x01UL;
  uint32_t id1_regVal = (id1 << 3) | (RTR_bit_1 << 1) | (ide << 2);

  uint8_t RTR_bit_2 = (Filter_RTR & (0x01UL << 1));
  RTR_bit_2 = (RTR_bit_2 >> 1) & 0x01UL;
  uint32_t id2_regVal = (id2 << 3) | (RTR_bit_2 << 1) | (ide << 2);

  pFilter->IdHigh = (id1_regVal >> 16) & 0xFFFF;
  pFilter->IdLow = (id1_regVal) & 0xFFFF;
  pFilter->MaskIdHigh = (id2_regVal >> 16) & 0xFFFF;
  pFilter->MaskIdLow = (id2_regVal) & 0xFFFF;
}


void Cus_CAN_Filter_SetStdMask32( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint16_t id1, uint16_t id1_mask )
{
  if ( !pFilter || id1 > 0x7FF || id1_mask > 0x7FF )  return;

  uint32_t RTR_bit;
  uint32_t RTR_Force_Match;
  uint32_t ide_bit = 0;
  uint32_t ide_bit_Force_Match = 1;
  if ( Filter_MASK_RTR == CAN_FILTER_MASK_DATA )   
  {
    RTR_Force_Match = 1;                  // 不匹配遥控帧. 强制RTR位为0,只匹配数据帧.
    RTR_bit = 0;
  }
  else if ( Filter_MASK_RTR == CAN_FILTER_MASK_REMOTE )
  {
    RTR_Force_Match = 1;                    // 只匹配遥控帧. 强制RTR为1.
    RTR_bit = 1;
  }
  else if ( Filter_MASK_RTR == (CAN_FILTER_MASK_REMOTE | CAN_FILTER_MASK_DATA) )
  {
    RTR_Force_Match = 0;                    // 遥控帧与数据帧均匹配.
    RTR_bit = 0;
  }
  else 
  {
    // 无效模式，默认只收数据帧
    RTR_Force_Match = 1;
    RTR_bit = 0;
  }

  uint32_t id_regVal = ((uint32_t)id1 << 21) | (RTR_bit << 1) | (ide_bit << 2);
  uint32_t mask_regVal = ((uint32_t)id1_mask << 21) | (RTR_Force_Match << 1) | (ide_bit_Force_Match << 2);

  pFilter->IdHigh = (id_regVal >> 16) & 0xFFFF;
  pFilter->IdLow = (id_regVal) & 0xFFFF;
  pFilter->MaskIdHigh = (mask_regVal >> 16) & 0xFFFF;
  pFilter->MaskIdLow = (mask_regVal) & 0xFFFF;
}


void Cus_CAN_Filter_SetExtMask32( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint32_t id1, uint32_t id1_mask )
{
  if ( !pFilter || id1 > 0x1FFFFFFF || id1_mask > 0x1FFFFFFF )   return;

  uint32_t RTR_bit;
  uint32_t RTR_Force_Match;
  uint32_t ide_bit = 1;
  uint32_t ide_bit_Force_Match = 1;

  if ( Filter_MASK_RTR == CAN_FILTER_MASK_DATA )   
  {
    RTR_Force_Match = 1;                  // 不匹配遥控帧. 强制RTR位为0,只匹配数据帧.
    RTR_bit = 0;
  }
  else if ( Filter_MASK_RTR == CAN_FILTER_MASK_REMOTE )
  {
    RTR_Force_Match = 1;                    // 只匹配遥控帧. 强制RTR为1.
    RTR_bit = 1;
  }
  else if ( Filter_MASK_RTR == (CAN_FILTER_MASK_REMOTE | CAN_FILTER_MASK_DATA) )
  {
    RTR_Force_Match = 0;                    // 遥控帧与数据帧均匹配.
    RTR_bit = 0;
  }
  else 
  {
    // 无效模式，默认只收数据帧
    RTR_Force_Match = 1;
    RTR_bit = 0;
  }

  uint32_t id1_regVal = (id1 << 3) | (RTR_bit << 1) | (ide_bit << 2);
  uint32_t mask_regVal = (id1_mask << 3) | (RTR_Force_Match << 1) | (ide_bit_Force_Match << 2);

  pFilter->IdHigh = (id1_regVal >> 16) & 0xFFFF;
  pFilter->IdLow = (id1_regVal) & 0xFFFF;
  pFilter->MaskIdHigh = (mask_regVal >> 16) & 0xFFFF;
  pFilter->MaskIdLow = (mask_regVal) & 0xFFFF;
}


void Cus_CAN_Filter_SetStdMask16( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint16_t id1, uint16_t id1_mask, uint16_t id2, uint16_t id2_mask )
{
  if ( !pFilter || id1 > 0x7FF || id1_mask > 0x7FF || id2 > 0x7FF || id2_mask > 0x7FF )   return;

  uint32_t ide_bit = 0;
  uint32_t ide_bit_Force_Match = 1;

  uint32_t RTR_bit;
  uint32_t RTR_Force_Match;
  if ( Filter_MASK_RTR == CAN_FILTER_MASK_DATA )   
  {
    RTR_Force_Match = 1;                  // 不匹配遥控帧. 强制RTR位为0,只匹配数据帧.
    RTR_bit = 0;
  }
  else if ( Filter_MASK_RTR == CAN_FILTER_MASK_REMOTE )
  {
    RTR_Force_Match = 1;                    // 只匹配遥控帧. 强制RTR为1.
    RTR_bit = 1;
  }
  else if ( Filter_MASK_RTR == (CAN_FILTER_MASK_REMOTE | CAN_FILTER_MASK_DATA) )
  {
    RTR_Force_Match = 0;                    // 遥控帧与数据帧均匹配.
    RTR_bit = 0;
  }
  else 
  {
    // 无效模式，默认只收数据帧
    RTR_Force_Match = 1;
    RTR_bit = 0;
  }

  uint32_t id1_regVal = ((uint32_t)id1 << 5) | (RTR_bit << 4) | (ide_bit << 3);
  uint32_t id1_maskregVal = ((uint32_t)id1_mask << 5) | (RTR_Force_Match << 4) | (ide_bit_Force_Match << 3);

  uint32_t id2_regVal = ((uint32_t)id2 << 5) | (RTR_bit << 4) | (ide_bit << 3);
  uint32_t id2_maskregVal = ((uint32_t)id2_mask << 5) | (RTR_Force_Match << 4) | (ide_bit_Force_Match << 3);

  pFilter->IdHigh = (id1_regVal);
  pFilter->IdLow = (id2_regVal);
  pFilter->MaskIdHigh = (id1_maskregVal);
  pFilter->MaskIdLow = (id2_maskregVal);
}


/**
 * @brief 注册用户提供的缓冲区作为接收环形缓冲区
 * @param pDev    指向 CAN 设备对象的指针
 * @param pBuffer 用户提供的缓冲区起始地址，大小至少为 sizeof(Cus_CAN_RxMsg_t)
 * @param size    缓冲区总大小（字节）
 * @return 0     成功
 * @return 0xFF  失败（参数无效、缓冲区过小或内存不足）
 *
 * @note 该函数将用户提供的缓冲区注册为接收环形缓冲区，用于中断接收模式。
 *       注册后，CAN 接收中断会将收到的消息存入此缓冲区，用户可通过
 *       Receive_IT 函数读取。
 *       缓冲区大小需至少能容纳一条消息（sizeof(Cus_CAN_RxMsg_t)），
 *       建议大小为消息结构体大小的整数倍，以获得最大利用率。
 *       注册操作应在使能接收中断之前完成，否则中断到来时可能无可用缓冲区。
 *       缓冲区内容在注册时会被清零，且由用户负责分配和管理（驱动不会释放）。
 *
 * @warning 若同时使用 FIFO0 和 FIFO1 的中断，两者会共用同一缓冲区，
 *          消息将按到达顺序混合存储。若需区分来源，请自行设计。
 */
static uint8_t Cus_CAN_registerRXBuffer( Cus_CAN_Device_t *pDev, void *pBuffer, uint32_t size )
{
  if ( !pDev || !pDev->canHandle || !pBuffer || size == 0 || size > UINT32_MAX )  return 0xFF;

  Cus_CAN_Priv_t *pPri = (Cus_CAN_Priv_t *)pDev->private;
  if ( !pPri )    return 0xFF;

  if ( size < sizeof(Cus_CAN_RxMsg_t) )   return 0xFF;  // 检查缓冲区大小是否至少能容纳一个消息.

  pPri->buffer_Size = size;
  pPri->max_msgNum = size / sizeof(Cus_CAN_RxMsg_t);
  pPri->head = 0;
  pPri->tail = 0;
  pPri->pRing = (Cus_CAN_RxMsg_t *)pBuffer;
  pPri->RxBuffer = pBuffer;
  memset(pPri->RxBuffer, 0, size);

  return 0;
}


/**
 * @brief 中断回调中调用，将收到的 CAN 消息存入环形缓冲区
 * @param pDev 指向 CAN 设备对象的指针
 * @param FIFO 接收 FIFO 标识（CAN_RX_FIFO0 或 CAN_RX_FIFO1）
 *
 * @note 该函数由 HAL_CAN_RxFifo0MsgPendingCallback 或 HAL_CAN_RxFifo1MsgPendingCallback
 *       等中断回调中调用，用于将收到的消息写入用户注册的环形缓冲区。
 *       调用前需确保已通过 Cus_CAN_registerRXBuffer 注册了有效缓冲区。
 *       若缓冲区已满，该函数会读取一次 FIFO 以清除中断标志（丢弃该消息），
 *       并调用 Cus_CANRecvITFull_Hook 通知用户。
 *       若读取失败，则调用 Cus_CANRecvITFailed_Hook。
 *       该函数应仅在中断上下文中执行，且要求 RX0 和 RX1 中断优先级相同，
 *       以避免环形缓冲区 head 指针竞争。
 *
 * @warning 用户不应手动调用此函数，它由驱动内部的中断回调自动触发。
 *          使用前务必先注册缓冲区，否则函数会直接返回，导致中断标志无法清除（可能死循环）。
 */
void Cus_CAN_RingRecvIT( Cus_CAN_Device_t *pDev, uint32_t FIFO )
{
  if ( !pDev || !pDev->canHandle )  return;

  if ( ((Cus_CAN_Priv_t *)pDev->private)->RxBuffer == NULL || ((Cus_CAN_Priv_t *)pDev->private)->pRing == NULL )  // 用户未注册缓冲区.
  {
    Cus_CAN_RxMsg_t dummy_msg;
    HAL_CAN_GetRxMessage(pDev->canHandle, FIFO, &dummy_msg.RxHeader, dummy_msg.RxData);

    return;   // 缓冲区未注册. 读取FIFO清中断后直接返回.
  }

  Cus_CAN_Priv_t *pPrivate = (Cus_CAN_Priv_t *)pDev->private;
  uint16_t next_head = ( pPrivate->head + 1 ) % pPrivate->max_msgNum; // 计算下一个写入位置.
  if ( next_head == pPrivate->tail )
  {
    #if (!USE_DEFAULT_RxFIFO_FULL_HOOK)
      // 缓冲区写满. 数据丢失.
      // 读取一次FIFO. 清除中断.
      Cus_CAN_RxMsg_t dummy_msg;
      HAL_CAN_GetRxMessage(pDev->canHandle, FIFO, &dummy_msg.RxHeader, dummy_msg.RxData);
      Cus_CANRecvITFull_Hook( pDev );
      return;
    #endif 

    #if (USE_DEFAULT_RxFIFO_FULL_HOOK)
      // 自定义处理机制.
      Cus_CANRecvITFull_Hook( pDev );
      next_head = ( pPrivate->head + 1 ) % pPrivate->max_msgNum;;    // Default Hook 内部由于重置了状态. 所以此处重新计算next_head. 确保下次写入逻辑偏移正确.
    #endif 
  }

  Cus_CAN_RxMsg_t *Msg = &pPrivate->pRing[pPrivate->head];
  if ( HAL_CAN_GetRxMessage(pDev->canHandle, FIFO, &Msg->RxHeader, Msg->RxData) != HAL_OK )
  {
    Cus_CANRecvITFailed_Hook(pDev);
    return;
  }
  pPrivate->head = next_head;
}



/**
 * @brief 快速配置 CAN（默认普通模式，500kbps）
 * @note 弱函数，用户可覆盖。可自定义快速启动配置。
 */
__attribute__((used)) __weak HAL_StatusTypeDef Cus_CAN_QuickConfig( CAN_TypeDef *instance, const Cus_CAN_GPIO_t *g_gpio )
{
  if ( !instance || !g_gpio )   return HAL_ERROR;

  #if (CAN_CFG_ALLOC_DYNAMIC)
    CANInitConfig_t *pInit;
    if ( Factory_CANInitConfig_t(&pInit) != 0 )   return HAL_ERROR;
  #elif (!CAN_CFG_ALLOC_DYNAMIC)
    uint8_t canInitBuf[sizeof(CANInitConfig_t) + 2];    // +2的冗余空间.

    int8_t ret = Factory_CANInitConfig_t_Static(canInitBuf, sizeof(canInitBuf));
    if ( ret )    return HAL_ERROR;

    CANInitConfig_t *pInit = (CANInitConfig_t *)canInitBuf;
  #endif  // CAN_CFG_ALLOC_DYNAMIC

  pInit->baudrate = CAN_BAUDRATE_500K;
  pInit->Instance = instance;
  pInit->Mode = MODE_NORMAL;
  pInit->is_AutoBusOff = false;
  pInit->is_AutoRestransmission = false;
  pInit->is_AutoWakeUP = false;
  pInit->is_ReceiveFifoLocked = false;
  pInit->is_TimeTriggeredMode = false;
  pInit->is_TransmitFifoPriority = false;
  pInit->SJW = Cus_CAN_SJW_1Tq;

  memcpy(&pInit->CAN_gpio, g_gpio, sizeof(Cus_CAN_GPIO_t));

  HAL_StatusTypeDef hReturn = pInit->Cus_CAN_Init((const CANInitConfig_t *)pInit);

  pInit->Self_Release(&pInit);      // 释放配置结构体. 不再需要.

  return hReturn;
}



/**
 * @brief 快速配置 过滤器（默认全通 32位宽，接收所有帧）
 * @note 弱函数，用户可覆盖。可自定义快速启动配置。
 */
__attribute__((used)) __weak HAL_StatusTypeDef Cus_Filter_QuickConfig( CAN_TypeDef *instance )
{
  if ( !instance )    return HAL_ERROR;

  CAN_HandleTypeDef *hcan = Cus_CAN_getHandle(instance);    // 此处获取Handle只为检查Instance所对应的CAN Handle是否已正确通过CUS库进行维护和初始化.
  if ( hcan == NULL )   return HAL_ERROR;

  #if (CAN_CFG_ALLOC_DYNAMIC)
    CANFilterConfig_t *pFilter;
    if ( Factory_CANFilterConfig_t(&pFilter) != 0 )   return HAL_ERROR;
  #elif (!CAN_CFG_ALLOC_DYNAMIC)
    uint8_t canFilterBuf[sizeof(CANFilterConfig_t) + 2];

    int ret = Factory_CANFilterConfig_t_Static(canFilterBuf, sizeof(canFilterBuf));
    if ( ret )    return HAL_ERROR;

    CANFilterConfig_t *pFilter = (CANFilterConfig_t *)canFilterBuf;
  #endif // CAN_CFG_ALLOC_DYNAMIC

  pFilter->FIFOAssignment = Cus_CAN_FIFOASSIGNMENT_FIFO0;
  pFilter->FilterBank = 0;
  pFilter->IdHigh = 0x0;
  pFilter->IdLow = 0x0;
  pFilter->MaskIdHigh = 0x0;
  pFilter->MaskIdLow = 0x0;
  pFilter->Mode = Cus_CAN_FILTERMODE_IDMASK;
  pFilter->Scale = Cus_CAN_SCALE_32BIT;
  pFilter->is_Activation = Cus_FILTER_Enable;

  HAL_StatusTypeDef hReturn = pFilter->Cus_CAN_FilterInit((const CANFilterConfig_t *)pFilter, instance);

  pFilter->Self_Release(&pFilter);

  return hReturn;
}



/**
 * @brief 一键配置CAN启动
 * @note 弱函数，用户可覆盖。可自定义快速启动配置。
 */
__attribute__((used)) __weak HAL_StatusTypeDef Cus_CAN_QuickSetup( CAN_TypeDef *instance, const Cus_CAN_GPIO_t *g_gpio )
{
  if ( !instance || !g_gpio )   return HAL_ERROR;
  
  HAL_StatusTypeDef hReturn;
  hReturn = Cus_CAN_QuickConfig(instance, g_gpio);
  if ( hReturn != HAL_OK )  return HAL_ERROR;

  hReturn = Cus_Filter_QuickConfig(instance);
  if ( hReturn != HAL_OK )  return HAL_ERROR;

  hReturn = Cus_CAN_Start(instance);

  return hReturn;
}



/**
 * @brief 配置 CAN 中断的 NVIC 优先级（弱函数，可覆盖）
 * @param pDev 指向 CAN 设备对象的指针
 *
 * @note 该函数由驱动在使能中断时自动调用（例如在 Cus_CAN_EnbIT 中），
 *       用户不应手动调用此函数，否则可能导致 NVIC 配置与中断使能状态不一致。
 *
 * @note 默认实现根据当前已使能的中断类型（通过 CheckInterrupt 查询 IER 寄存器）
 *       动态开启对应的 NVIC 中断向量，并为每个向量设置优先级 5（抢占优先级 5，子优先级 0）.
 *
 * @note 用户可以覆盖此函数以自定义中断优先级、增加或减少需要使能的 NVIC 通道.
 *       覆盖后，请确保在函数内根据实际需求配置 NVIC，且不要手动调用原函数.
 *
 * @warning 不建议手动调用此函数，因为驱动会在合适的时机自动调用它.
 * 
 * @warning 如果同时使用 FIFO0 和 FIFO1 的中断，必须为这两个中断设置相同的优先级，
 *          否则可能会导致环形缓冲区数据错乱。默认实现中两者均为优先级 5，更改时请格外注意.
 */
__attribute__((used)) __weak void Cus_CAN_NVIC_Config( Cus_CAN_Device_t *pDev )
{
  #warning "Ensure RX0 and RX1 interrupts have same priority to avoid ring buffer corruption."

  if ( !pDev || !pDev->canHandle )  return;

  if ( pDev->Instance == CAN1 )
  {
    // 检查是否使能  接收中断.
    bool is_RecvRx0_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev ,CAN_IT_RX_FIFO0_MSG_PENDING);
    bool is_RecvRx0Full_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_RX_FIFO0_FULL);
    bool is_RecvRx0OverRun_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_RX_FIFO0_OVERRUN);
    if ( is_RecvRx0_IT || is_RecvRx0Full_IT || is_RecvRx0OverRun_IT )
    {
      HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
      HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    }

    bool is_RecvRx1_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_RX_FIFO1_MSG_PENDING);
    bool is_RecvRx1Full_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_RX_FIFO1_FULL);
    bool is_RecvRx1OverRun_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_RX_FIFO1_OVERRUN);
    if ( is_RecvRx1_IT || is_RecvRx1Full_IT || is_RecvRx1OverRun_IT )
    {
      HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);
      HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    }

    bool is_TxMailBoxCplt_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_TX_MAILBOX_EMPTY);
    if ( is_TxMailBoxCplt_IT )
    {
      HAL_NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, 5, 0);
      HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    }

    bool is_Error_WakeUP_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_WAKEUP);
    bool is_Error_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_ERROR);
    bool is_Error_Sleep_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_SLEEP_ACK);
    bool is_Error_Passive_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_ERROR_PASSIVE);
    bool is_Error_Busoff_IT = pDev->CheckInterrupt((const Cus_CAN_Device_t *)pDev, CAN_IT_BUSOFF);
    if ( is_Error_WakeUP_IT || is_Error_IT || is_Error_Sleep_IT || is_Error_Passive_IT || is_Error_Busoff_IT )
    {
      HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 5, 0);
      HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
    }

  }
  #if defined(CAN2)
    if ( pDev->Instance == CAN2 )
    {
      /* 待实现 */
    }
  #endif // CAN2

  #if defined(CAN3)
    if ( pDev->Instance == CAN3 )
    {
      /* 待实现 */
    }
  #endif // CAN3
}


/* ------------------------------------------------------------------------- */


/* ----------------------------- Send_ASYNC --------------------------------- */
#if (USE_SEND_ASYNC)
  static void Cus_CAN_NodePollInit( Cus_CAN_Device_t *pDev )
  {
    if ( !pDev || !pDev->Instance )   return;

    Cus_CAN_Priv_t *pPriv = (Cus_CAN_Priv_t *)pDev->private;
    pPriv->TxBusy = 1;      pPriv->TxHead = NULL;
    pPriv->TxTail = NULL;   pPriv->TxCurrent = NULL;

    // 由于内存池由所有CAN实例共享，所以无论哪个实例初始化时都会初始化全局内存池（只一次）.
    if ( !is_NodePollInit )        
    {
      TxFreeStackIndex = 0;
      FreeStackCount = TX_NODE_POLL_SIZE;
      is_NodePollInit = true;
    }

    for( uint8_t i = 0; i < TX_NODE_POLL_SIZE; i++ )
    {
      s_TxNodePoll[i].next = NULL;
      s_TxNodePoll[i].canIndex = CAN_FREE;
      memset(s_TxNodePoll[i].data, 0, sizeof(s_TxNodePoll[i].data));
      memset(&s_TxNodePoll[i].TxHeader, 0, sizeof(CAN_TxHeaderTypeDef));

      s_TxNodeFreeStack[TxFreeStackIndex++] = &s_TxNodePoll[i];     // 将节点池中节点依次压栈空闲指针栈(初始时，所有节点空闲).
    }
  }


  static TxMsgNode_t *Cus_CAN_NodeAlloc( Cus_CAN_Device_t *pDev )
  {
    if ( !pDev || !pDev->Instance )   return NULL;

    Cus_CAN_Priv_t *pPri = (Cus_CAN_Priv_t *)pDev->private;

    // 先检查空闲栈是否仍可用.
    if ( FreeStackCount == 0 )
    {
      // 节点池已全部被分配,无空余空间. 返回空指针由上层进行处理.
      return NULL;
    }

__disable_irq();
    // 获取新节点.
    TxMsgNode_t *pNode = s_TxNodeFreeStack[--TxFreeStackIndex];   // 这里一定要是前置自减！
    pNode->canIndex = Cus_CAN_getIndex(pDev->Instance);
    FreeStackCount--;
__enable_irq();

    return pNode;
  }


  static void Cus_CAN_FreeNode( TxMsgNode_t *pNode )
  {
    if ( !pNode )   return;

    pNode->canIndex = CAN_FREE;
    pNode->next = NULL;

    // 清除残留数据.
    memset(pNode->data, 0, sizeof(pNode->data));
    memset(&pNode->TxHeader, 0, sizeof(CAN_TxHeaderTypeDef));

    // 重新放回空闲指针栈.(此处操作全局数据无需额外显示临界保护. 因为上层调用已经确保处于临界状态)
    s_TxNodeFreeStack[TxFreeStackIndex++] = pNode;
    FreeStackCount++;
  }


  static HAL_StatusTypeDef Cus_CAN_Send_IT( Cus_CAN_Device_t *pDev, CAN_TxHeaderTypeDef Txheader, uint8_t *Send_Buf )
  {
    if ( !pDev || !pDev->Instance || !Send_Buf )    return HAL_ERROR;

    Cus_CAN_Priv_t *pPriv = (Cus_CAN_Priv_t *)pDev->private;

    TxMsgNode_t *Send_Node = Cus_CAN_NodeAlloc(pDev);
    if ( !Send_Node )   return HAL_BUSY;    // 分配失败. 无多余空闲节点. 返回BUSY供上层检查处理.

    // 填充 Node 的必须字段.
    memcpy(Send_Node->data, Send_Buf, sizeof(Send_Node->data));
    Send_Node->TxHeader = Txheader;

__disable_irq();
    // 判断当前节点是否是队列第一个元素.
    if ( pPriv->TxHead == NULL && pPriv->TxTail == NULL )
    {
      // 对于队列中第一个成员. Head Tail 都是它自身.
      pPriv->TxHead = Send_Node;
      pPriv->TxTail = Send_Node;
    }
    else 
    {
      // 将当前发送节点挂载到发送队列末尾.
      pPriv->TxTail->next = Send_Node;

      // 更新队列末尾成员.
      pPriv->TxTail = Send_Node;
    }
__enable_irq();

    // 从队列头取节点,启动一次发送. (使用邮箱0)
    if ( pPriv->TxBusy )
    {
      // 取出队列头作为要发送的节点.
      pPriv->TxCurrent = pPriv->TxHead;

      // 更新头部节点.
      pPriv->TxHead = pPriv->TxHead->next;

      if ( pPriv->TxHead == NULL )  pPriv->TxTail = NULL;

	  uint32_t TxMailbox;
      HAL_StatusTypeDef hReturn = HAL_CAN_AddTxMessage(pDev->canHandle, &pPriv->TxCurrent->TxHeader, pPriv->TxCurrent->data, &TxMailbox);
      if ( hReturn != HAL_OK )
      {
        // 请求失败处理.(待实现)
        return HAL_ERROR;
      }
      pPriv->TxBusy = 0;
    }

    return HAL_OK;
  }


  void Cus_CAN_ProcessTxQueue( Cus_CAN_Device_t *pDev )
  {
    if ( !pDev || !pDev->Instance )   return;

    Cus_CAN_Priv_t *pPriv = (Cus_CAN_Priv_t *)pDev->private;

__disable_irq();
    if ( pPriv->TxHead )
    {
      // 将当前已发送完成的节点0初始化后重新放回空闲指针栈.
      Cus_CAN_FreeNode(pPriv->TxCurrent);   

      // 更新当前的 Current. 取出头节点准备启动下一次发送.
      pPriv->TxCurrent = pPriv->TxHead;

      // 更新Head指向下一个待发送节点.
      pPriv->TxHead = pPriv->TxHead->next;

      if ( !pPriv->TxHead )    pPriv->TxTail = NULL;   // 发送队列空.

	  uint32_t TxMailbox;
      HAL_CAN_AddTxMessage(pDev->canHandle, &pPriv->TxCurrent->TxHeader, pPriv->TxCurrent->data, &TxMailbox);
__enable_irq();
      return;

    }

    // 释放最后一个节点.
    Cus_CAN_FreeNode(pPriv->TxCurrent);
    pPriv->TxCurrent = NULL;  

    // 整个队列发送完毕.才置空闲标志位.
    pPriv->TxBusy = 1;    
__enable_irq();
  }
#endif // USE_SEND_ASYNC
/* -------------------------------------------------------------------------- */



/* -------------------------------- Default Feature ------------------------------------ */
#if (USE_DEFAULT_RxFIFO_FULL_HOOK)
  void Cus_CANRecvITFull_Hook( Cus_CAN_Device_t *pDev )
  {
    if ( !pDev || !pDev->canHandle )    return;

    if ( pDev->RingFIFOFull_Count >= MAX_FIFO_FULL_COUNT )   return;

    Cus_CAN_Priv_t *Private = (Cus_CAN_Priv_t *)pDev->private;
    size_t total_size = Private->buffer_Size + 2 * sizeof(uint16_t);  

#warning "if malloc failed. Plz check your heap_size."
    uint8_t * const pBackup = (uint8_t *)malloc(total_size);   // 多开辟四个4字节用于存储 head tail等关键信息.
    if ( !pBackup )   return;       // 空间申请失败. 处理失效.

    uint8_t *pWrite = pBackup;
    *(volatile uint16_t *)pWrite = Private->head;
    pWrite += sizeof(uint16_t);

    *(volatile uint16_t *)pWrite = Private->tail;
    pWrite += sizeof(uint16_t);

    memcpy(pWrite, Private->RxBuffer, Private->buffer_Size);
    // memset(Private->RxBuffer, 0, Private->buffer_Size);  // 拷贝后清除原缓冲区数据(可选，为了性能考虑此处关闭).

    // 状态复原.
    Private->head = 0;
    Private->tail = 0;
    pDev->pBackUPRecv_Buf[pDev->RingFIFOFull_Count] = (void *)pBackup;
    pDev->RingFIFOFull_Count++;
    pDev->is_RingFIFO_Full = true;
  }
#endif // USE_DEFAULT_RxFIFO_FULL_HOOK
/* ------------------------------------------------------------------------------------- */


#ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
  U8 Cus_CanTP_canSendFunc_Asynchronous( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc )
  {
    if ( !pConn || !data || dlc < 8 )   return 0;  // 用于采用填充机制. 所以dlc(数据场长度)必须 >= 8. == 8为经典CAN2.0. > 8 为CAN FD形式.

    CAN_TypeDef *CANInstance = (CAN_TypeDef *)pConn->BindCANDevice;
    Cus_CAN_Device_t * pDev = Cus_CAN_getControlBlock(CANInstance);
    if ( !pDev )  return 0;   // Device 对象获取失败. 返回.

    CAN_TxHeaderTypeDef TxHeader;
    TxHeader.DLC = dlc;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.StdId = (canId & 0x7FF);
    TxHeader.TransmitGlobalTime = DISABLE;
    TxHeader.IDE = CAN_ID_STD;    // 先写死STD标准11位ID模式. 后续待拓展为 29位 EXT拓展ID模式.

    U32 Txmailbox;
    HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(pDev->canHandle, &TxHeader, data, &Txmailbox); 
    if ( ret != HAL_OK )    return 0;   // 发送失败. 无空闲邮箱或硬件错误.

    // 记录使用的邮箱号到连接句柄（用于中断回调匹配）
    pConn->TxMailBoxIndex = (U8)Txmailbox;

    return 1;
  }


  U8 Cus_CanTP_canRecvFunc_Asynchronous( Cus_CANTp_Conn_t *pConn, U32 *pcanId, U8 *pData, U8 *pDlc )
  {
    if ( !pConn || !pConn->BindCANDevice || !pcanId || !pData || !pDlc )   return 0;

    CAN_TypeDef *Instance = (CAN_TypeDef *)pConn->BindCANDevice;
    Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(Instance);
    if ( !pDev )  return 0;

    CAN_RxHeaderTypeDef RxHeader;
    U8 rxdata[8];                   // 经典CAN. 8帧数据段.
    if ( pDev->Receive_IT && pDev->Receive_IT(pDev, &RxHeader, rxdata) == HAL_OK )
    {
      if ( RxHeader.IDE == CAN_ID_STD )   *pcanId = RxHeader.StdId;

      *pDlc = RxHeader.DLC;
      memcpy(pData, rxdata, RxHeader.DLC);
      return 1;
    }

    return 0;
  }
#endif // __Cus_CANTP_XzzwY7a9BBCTQ7__

