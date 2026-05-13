#ifndef __CAN_CUS_H__
#define __CAN_CUS_H__


/* ******************************************* */
  /* 
    用户必须在使用本库前包含正确的 STM32 头文件.
    - 对于 F1 系列： #include "stm32f1xx_hal.h".
    - 对于 F4 系列： #include "stm32f4xx_hal.h".
    - 已经给出了 F1 与 F4 系列的实现,若芯片属于其它型号,请自行包含其相关的HAL库文件.
  */
  #include "stm32f1xx_hal.h"    
      #define STM32F1xx

  // #include "stm32f4xx_hal.h"
      // #define STM32F4xx

  // ....
  // #define STM32ADVANCE             // 注意：若芯片为其它(高于F4系列)时，请在自行添加相关hal头文件后，启用该宏并注释掉
                                          // #include "stm32f1xx_hal.h" 与 #define STM32F1xx

/* ******************************************* */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "Cus_CANTP.h"


/* *************** Feature ****************** */
  #define USE_DEFAULT_RxFIFO_FULL_HOOK    (0)
    #if (USE_DEFAULT_RxFIFO_FULL_HOOK)
      #define MAX_FIFO_FULL_COUNT  (5)
    #endif // USE_DEFAULT_RxFIFO_FULL_HOOK

  #define CAN_CFG_ALLOC_DYNAMIC           (0)     // 配置相关结构体内存分配方式: 0=静态分配(默认), 1=动态分配.
  #define CAN_TCB_ALLOC_DYNAMIC           (0)     // 设备结构体内存分配方式: 0=静态分配(默认), 1=动态分配.
  #define USE_SEND_ASYNC                  (0)     // 是否启用异步发送: 0=不启用异步发送(发送为阻塞式). 1=启用异步发送(添加异步发送API).
    #if (USE_SEND_ASYNC)
      #define SEND_ASYNC_NodePOLL_DYNAMIC (0)     // 发送队列节点池内存分配方式: 0=静态分配(默认). 1=动态分配.
    #endif // USE_SEND_ASYNC


/* ******************************************* */


/* *************** Define ****************** */
  #define MAX_SUPPORT_CANDEV             (3)
  #define CAN1_INDEX                     (0)
  #define CAN2_INDEX                     (1)
  #define CAN3_INDEX                     (2)

  #define CAN_FILTER_RTR_NONE            (0)
  #define CAN_FILTER_RTR_ID1             (1UL << 0)
  #define CAN_FILTER_RTR_ID2             (1UL << 1)
  #define CAN_FILTER_RTR_ID3             (1UL << 2)
  #define CAN_FILTER_RTR_ID4             (1UL << 3)
  #define CAN_FILTER_RTR_ALL             (CAN_FILTER_RTR_ID1 | CAN_FILTER_RTR_ID2 | CAN_FILTER_RTR_ID3 | CAN_FILTER_RTR_ID4)

  #define CAN_FILTER_MASK_DATA           (0xAA)
  #define CAN_FILTER_MASK_REMOTE         (0xBB)

  #if (USE_SEND_ASYNC) && (!SEND_ASYNC_NodePOLL_DYNAMIC)
    #define TX_NODE_POLL_SIZE            (8)        // 队列节点池内存预分配大小(单位: sizeof(TxMsgNode_t)). 仅用于静态内存分配时.
  #endif 

  #if (USE_SEND_ASYNC)
    #define CAN_FREE                     (-1)       // 标志该节点目前未绑定实例. 处于空闲状态.
  #endif 
/* ***************************************** */


/* ******************************************* */

#if (USE_SEND_ASYNC)
  typedef struct TxMsgNode              // 发送队列消息节点(链表).
  {
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t data[8];
    int8_t canIndex;                   // 记录当前正在发送的CAN实例.
    struct TxMsgNode *next;

  } TxMsgNode_t;
#endif // USE_SEND_ASYNC


typedef enum 
{
  MODE_NORMAL,
  MODE_LOOPBACK,
  MODE_SILENT,
  MODE_SILENT_LPBACK

} Cus_CAN_Mode_t;



typedef enum
{
  CAN_BAUDRATE_125K,
  CAN_BAUDRATE_250K,
  CAN_BAUDRATE_500K,
  CAN_BAUDRATE_1M

} Cus_CAN_Baudrate_t;


typedef enum
{
  Cus_CAN_FILTERMODE_IDMASK,
  Cus_CAN_FILTERMODE_IDLIST

} Cus_CANFilter_Mode_t;


typedef enum
{
  Cus_CAN_SCALE_16BIT,
  Cus_CAN_SCALE_32BIT

} Cus_CANFilter_Scale_t;


typedef enum
{
  Cus_CAN_FIFOASSIGNMENT_FIFO0,
  Cus_CAN_FIFOASSIGNMENT_FIFO1

} Cus_CANFIFOASS_t;


typedef enum
{
  Cus_FILTER_Enable,
  Cus_FILTER_Disable

} Cus_CAN_FilterEnb_t;


typedef enum 
{
  Cus_CAN_SJW_1Tq,
  Cus_CAN_SJW_2Tq,
  Cus_CAN_SJW_3Tq,
  Cus_CAN_SJW_4Tq

} Cus_CAN_SJW_t;


typedef struct 
{
  GPIO_TypeDef *CAN_GPIOPort_x;
  uint16_t CAN_GPIO_RX; 
  uint16_t CAN_GPIO_TX;
  uint8_t Alternate;   /* 仅 F4 等系列使用，F1 忽略 */

} Cus_CAN_GPIO_t;


typedef struct CANInitConfig_t CANInitConfig_t;
struct CANInitConfig_t
{
  CAN_TypeDef *Instance;
  Cus_CAN_Mode_t Mode;
  Cus_CAN_Baudrate_t baudrate;
  Cus_CAN_GPIO_t CAN_gpio;
  Cus_CAN_SJW_t SJW;
  bool is_AutoRestransmission;
  bool is_AutoWakeUP;
  bool is_ReceiveFifoLocked;
  bool is_TimeTriggeredMode;
  bool is_TransmitFifoPriority;
  bool is_AutoBusOff;
  bool is_DynamicAlloc;

  HAL_StatusTypeDef (*Cus_CAN_Init)( const CANInitConfig_t *pConf_Structure );
  void (*Self_Release)( CANInitConfig_t **pConf );
};


typedef struct CANFilterConfig_t CANFilterConfig_t;
struct CANFilterConfig_t
{
  Cus_CANFilter_Mode_t Mode;
  Cus_CANFilter_Scale_t Scale;
  Cus_CANFIFOASS_t FIFOAssignment;
  uint8_t FilterBank;
  uint32_t  IdHigh;              
  uint32_t  IdLow;               
  uint32_t  MaskIdHigh;          
  uint32_t  MaskIdLow;       
  Cus_CAN_FilterEnb_t is_Activation;
  bool is_DynamicAlloc;

  HAL_StatusTypeDef (*Cus_CAN_FilterInit)( const CANFilterConfig_t *pFilterConf, CAN_TypeDef *instance );
  void (*Self_Release)( CANFilterConfig_t **pConf );
};


/* 环形缓冲区元素：一条完整的 CAN 消息 */
typedef struct 
{
  CAN_RxHeaderTypeDef RxHeader;
  uint8_t RxData[8];

} Cus_CAN_RxMsg_t;


typedef struct Cus_CAN_Device Cus_CAN_Device_t;
struct Cus_CAN_Device
{
  CAN_TypeDef *Instance;
  CAN_HandleTypeDef *canHandle;
  HAL_StatusTypeDef (*Send)( Cus_CAN_Device_t *pDev, CAN_TxHeaderTypeDef Txheader, uint8_t *Send_Buf );

  #if (USE_SEND_ASYNC)
    HAL_StatusTypeDef (*Send_IT)( Cus_CAN_Device_t *pDev, CAN_TxHeaderTypeDef Txheader, uint8_t *Send_Buf );
  #endif 

  HAL_StatusTypeDef (*Receive)( const Cus_CAN_Device_t *pDev, CAN_RxHeaderTypeDef *pHeader, uint8_t *Recv_Buf, uint32_t RxFifo );
  HAL_StatusTypeDef (*Receive_IT)( Cus_CAN_Device_t *pDev, CAN_RxHeaderTypeDef *pHeader, uint8_t *Recv_Buf );
  HAL_StatusTypeDef (*EnableInterrupt)( Cus_CAN_Device_t *pDev, uint32_t interrupt_mask );
  bool (*CheckInterrupt)( const Cus_CAN_Device_t *pDev, uint32_t interrupt_mask );
  uint8_t (*registerRxBuffer)( Cus_CAN_Device_t *pDev, void *pBuffer, uint32_t size );

  void *private;
  #if (USE_DEFAULT_RxFIFO_FULL_HOOK)
    void **pBackUPRecv_Buf;
    bool is_RingFIFO_Full;
    uint8_t RingFIFOFull_Count;
  #endif // USE_DEFAULT_RxFIFO_FULL_HOOK
};


typedef struct 
{
  uint32_t prescaler;
  uint32_t bs1;
  uint32_t bs2;
  float real_baudrate;
  uint32_t can_clock;

} Cus_CAN_RateInfo_t;



/* ******************************************* */



/* ----------------------------------------------------------------- */
#if (CAN_CFG_ALLOC_DYNAMIC)
  uint8_t Factory_CANInitConfig_t( CANInitConfig_t **pOutConfig );
  uint8_t Factory_CANFilterConfig_t( CANFilterConfig_t **pOutConfig );
#elif (!CAN_CFG_ALLOC_DYNAMIC)
  int8_t Factory_CANInitConfig_t_Static( uint8_t *buffer, uint32_t buffer_size );
  int8_t Factory_CANFilterConfig_t_Static( uint8_t *buffer, uint32_t buffer_size );
#endif // CAN_CFG_ALLOC_DYNAMIC

CAN_HandleTypeDef *Cus_CAN_getHandle( CAN_TypeDef *instance );
HAL_StatusTypeDef Cus_CAN_getRateInfo( CAN_TypeDef *instance, Cus_CAN_RateInfo_t *pOutInfo );
Cus_CAN_Device_t *Cus_CAN_getControlBlock( CAN_TypeDef *instance );
int8_t Cus_CAN_getIndex( CAN_TypeDef *instance );
void Cus_CAN_DeviceClose( Cus_CAN_Device_t **pDev );
HAL_StatusTypeDef Cus_CAN_Start( CAN_TypeDef *instance );

#if (USE_SEND_ASYNC)
  void Cus_CAN_ProcessTxQueue( Cus_CAN_Device_t *pDev );
#endif // USE_SEND_ASYNC
/* ----------------------------------------------------------------- */


/* ----------------------------------------------------------------- */
/** ************************************************************************************************************
 * @brief   CAN 过滤器ID配置辅助函数库
 * @details 一组用于配置 STM32 CAN 过滤器 ID 配置的便捷函数。
 *          支持列表模式（ID 列表）和掩码模式（ID+掩码），覆盖标准帧（11 位 ID）和扩展帧（29 位 ID）。
 *          所有函数仅填充 CANFilterConfig_t 结构体的 ID 和掩码字段.
 *
 * @note    使用前请确保已包含正确的 STM32 HAL 头文件（如 stm32f1xx_hal.h）。
 * @note    Filter_MASK_RTR 参数使用以下宏组合：
 *          - CAN_FILTER_MASK_DATA   : 只接收数据帧
 *          - CAN_FILTER_MASK_REMOTE : 只接收远程帧
 *          - 两者按位或 : 数据帧和远程帧都接收
 * 
 *          Filter_RTR 参数使用以下宏组合:
 *          - CAN_FILTER_RTR_ID1     : 列表模式下.ID1接收远程帧.
 *          - CAN_FILTER_RTR_ID2     : 列表模式下.ID2接收远程帧.
 *          - CAN_FILTER_RTR_ID3     : 列表模式下.ID3接收远程帧.
 *          - CAN_FILTER_RTR_ID4     : 列表模式下.ID4接收远程帧.
 *          - CAN_FILTER_RTR_NONE    : 列表模式下. 所有ID均接收数据帧.
 *          - CAN_FILTER_RTR_ALL     : 列表模式下. 所有ID均接收远程帧.
 * ************************************************************************************************************
 */

  /**
   * @brief   配置 32 位列表模式过滤器，接收两个标准 ID（11 位）。
   * @param   pFilter      指向 CANFilterConfig_t 结构体的指针，用于接收配置值。
   * @param   Filter_RTR   RTR 控制位掩码，低 2 位分别对应 id1 和 id2：
   *                       - bit0: id1 的 RTR 控制（0=数据帧，1=远程帧）
   *                       - bit1: id2 的 RTR 控制
   *                       可使用 CAN_FILTER_RTR_ID1 / CAN_FILTER_RTR_ID2 进行组合。
   * @param   id1          第一个标准 ID（0~0x7FF）。
   * @param   id2          第二个标准 ID（0~0x7FF）。
 */
    void Cus_CAN_Filter_SetStdList32( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint16_t id1, uint16_t id2 );
    
  /**
   * @brief   配置 16 位列表模式过滤器，接收四个标准 ID（11 位）。
   * @param   pFilter      指向 CANFilterConfig_t 结构体的指针。
   * @param   Filter_RTR   RTR 控制位掩码，低 4 位分别对应 id1~id4：
   *                       - bit0: id1, bit1: id2, bit2: id3, bit3: id4
   *                       每位 0=数据帧，1=远程帧。
   * @param   id1,id2,id3,id4  四个标准 ID（各 0~0x7FF）。
 */
    void Cus_CAN_Filter_SetStdList16( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint16_t id1, uint16_t id2, uint16_t id3, uint16_t id4 );

  /**
   * @brief   配置 32 位列表模式过滤器，接收两个扩展 ID（29 位）。
   * @param   pFilter      指向 CANFilterConfig_t 结构体的指针。
   * @param   Filter_RTR   RTR 控制位掩码，低 2 位分别对应 id1 和 id2（0=数据帧，1=远程帧）。
   * @param   id1          第一个扩展 ID（0~0x1FFFFFFF）。
   * @param   id2          第二个扩展 ID（0~0x1FFFFFFF）。
   * @note    过滤器会自动强制匹配 IDE=1（扩展帧），无需额外配置。
 */
    void Cus_CAN_Filter_SetExtList32( CANFilterConfig_t *pFilter, uint8_t Filter_RTR, uint32_t id1, uint32_t id2 );

  /**
   * @brief   配置 32 位掩码模式过滤器，匹配一个标准 ID（11 位）。
   * @param   pFilter         指向 CANFilterConfig_t 结构体的指针。
   * @param   Filter_MASK_RTR RTR 匹配模式，可选值：
   *                          - CAN_FILTER_MASK_DATA   : 只接收数据帧
   *                          - CAN_FILTER_MASK_REMOTE : 只接收远程帧
   *                          - 两者按位或 : 数据帧和远程帧都接收
   * @param   id1             期望匹配的标准 ID（0~0x7FF）。
   * @param   id1_mask        掩码，对应位为 1 表示必须匹配，为 0 表示忽略。
 */
    void Cus_CAN_Filter_SetStdMask32( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint16_t id1, uint16_t id1_mask );

  /**
   * @brief   配置 32 位掩码模式过滤器，匹配一个扩展 ID（29 位）。
   * @param   pFilter         指向 CANFilterConfig_t 结构体的指针。
   * @param   Filter_MASK_RTR RTR 匹配模式（同 SetStdMask32）。
   * @param   id1             期望匹配的扩展 ID（0~0x1FFFFFFF）。
   * @param   id1_mask        掩码，对应位为 1 表示必须匹配，为 0 表示忽略。
   * @note    过滤器会自动强制匹配 IDE=1（扩展帧），无需额外配置。
 */
    void Cus_CAN_Filter_SetExtMask32( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint32_t id1, uint32_t id1_mask );

  /**
   * @brief   配置 16 位掩码模式过滤器，匹配两个独立的标准 ID（11 位）。
   * @param   pFilter         指向 CANFilterConfig_t 结构体的指针。
   * @param   Filter_MASK_RTR RTR 匹配模式（同 SetStdMask32），**两个过滤器共用此设置,请注意**。
   * @param   id1, id1_mask   第一个过滤器的 ID 和掩码。
   * @param   id2, id2_mask   第二个过滤器的 ID 和掩码。
   * @note    两个过滤器独立工作，可分别匹配不同 ID。
 */
    void Cus_CAN_Filter_SetStdMask16( CANFilterConfig_t *pFilter, uint8_t Filter_MASK_RTR, uint16_t id1, uint16_t id1_mask, uint16_t id2, uint16_t id2_mask );

/* ----------------------------------------------------------------- */


/* ----------------------------------------------------------------- */
void Cus_FilterConfigFailed_Hook( CAN_HandleTypeDef *hcan, const CANFilterConfig_t *pFilterConf, HAL_StatusTypeDef hal_status );
void Cus_CANInitFailed_Hook( CAN_HandleTypeDef *hcan, const CANInitConfig_t *pInitConf, HAL_StatusTypeDef hal_status );
void Cus_CANStartFailed_Hook( CAN_HandleTypeDef *hcan, HAL_StatusTypeDef hal_status );
void Cus_CANSendFailed_Hook( Cus_CAN_Device_t *pDev, HAL_StatusTypeDef hal_status );
void Cus_CANRecvITFull_Hook( Cus_CAN_Device_t *pDev );
void Cus_CANRecvITFailed_Hook ( Cus_CAN_Device_t *pDev );

void Cus_CAN_NVIC_Config( Cus_CAN_Device_t *pDev );



/**
 * @brief 快速配置 CAN 外设（仅初始化，不含过滤器及启动）
 * @param instance CAN 外设实例（CAN1/CAN2/CAN3）
 * @param pGpio    GPIO 配置结构体指针，包含端口、RX/TX 引脚及 Alternate（F4 系列）
 * @return HAL_OK 成功，否则错误码
 * @note 默认参数：500kbps，普通模式，自动重传，FIFO 不锁定，按 ID 优先级发送。
 *       初始化完成后不会自动启动 CAN，需用户自行调用 Cus_CAN_Start()。
 *       若需接收数据，还需配置过滤器（可调用 Cus_Filter_QuickConfig）。
 */
__weak HAL_StatusTypeDef Cus_CAN_QuickConfig( CAN_TypeDef *instance, const Cus_CAN_GPIO_t *g_gpio );


/**
 * @brief 快速配置全通过滤器（接收所有帧）
 * @param instance CAN 外设实例
 * @return HAL_OK 成功，否则错误码
 * @note 配置为 32 位掩码模式，ID 及掩码全 0，关联 FIFO0，滤波器组 0，并启用。
 *       此函数需在 CAN 初始化后调用，过滤器生效前无需启动 CAN。通常用于快速设置
 *       测试环境。
 */
__weak HAL_StatusTypeDef Cus_Filter_QuickConfig( CAN_TypeDef *instance );


/**
 * @brief 一键完整配置（默认初始化 + 全通过滤器 + 启动 CAN）
 * @param instance CAN 外设实例
 * @param pGpio    GPIO 配置结构体指针
 * @return HAL_OK 成功，否则错误码
 * @note 依次调用 Cus_CAN_QuickConfig、Cus_Filter_QuickConfig 和 Cus_CAN_Start。
 *       适用于最简测试场景，用户无需额外操作即可开始收发。
 */
__weak HAL_StatusTypeDef Cus_CAN_QuickSetup( CAN_TypeDef *instance, const Cus_CAN_GPIO_t *g_gpio );

/* ----------------------------------------------------------------- */


/* 注意: 以下方法仅在包含 Cus配套的 CANTP 头文件后启用. */
#ifdef __Cus_CANTP_XzzwY7a9BBCTQ7__
  U8 Cus_CanTP_canSendFunc_Asynchronous( Cus_CANTp_Conn_t *pConn, U32 canId, const U8* data, U16 dlc );  // 异步

  U8 Cus_CanTP_canRecvFunc_Asynchronous( Cus_CANTp_Conn_t *pConn, U32 *pcanId, U8 *pData, U8 *pDlc );
#endif // __Cus_CANTP_XzzwY7a9BBCTQ7__

#endif // __CAN_CUS_H__
