#ifndef __CUS_CANTP_CFG_H__
#define __CUS_CANTP_CFG_H__


/*  ---------------------------------------------------  */
typedef unsigned char U8;
typedef unsigned int U32;
typedef unsigned short U16; 

typedef signed char S8;
typedef signed short S16;
typedef signed short S32;

#ifndef NULL
  #define NULL ((void *)0)
#endif
/*  ---------------------------------------------------  */

#define CHANNEL_CONFIG_TABLE_COUNT          (4)
#define BASE_CANTP_ADDR                     (0x700UL)
#define FUNCTIONAL_ID                       (0x7DFUL)

typedef struct Cus_CANTP_NAI 
{
  U8 TA;        // 目标地址.
  U8 SA;        // 源地址.
  U8 TA_Type;   // 地址类型 (0 = 物理寻址. 1 = 功能寻址. 其余无效).
  U8 AE;        // 地址拓展(混合寻址所用 默认为0).

} Cus_CANTP_NAI_t;


typedef struct Cus_CANTP_Cfg
{
  Cus_CANTP_NAI_t N_AI;   // 地址信息.
  U8 AddrMode;            // 寻址模式（0 = 普通寻址. 1 = 拓展寻址. 2 = 混合寻址）.
  U8 TxDLC;               // 对于经典CAN模式必须配置为8.
  U32 FunctionalCanID;    // 功能寻址地址.

} Cus_CANTP_Cfg_t;


Cus_CANTP_Cfg_t *Cus_Cantp_GetChannel( U8 ChannalIndex );

#endif // __CUS_CANTP_CFG_H__
