#include "Cus_CANTP_cfg.h"


Cus_CANTP_Cfg_t ChannelConfigTable[CHANNEL_CONFIG_TABLE_COUNT] = 
{
  [0] = {.N_AI.TA = 0, .N_AI.SA = 0, .N_AI.AE = 0, .N_AI.TA_Type = 0, .AddrMode = 0, .TxDLC = 8, .FunctionalCanID = FUNCTIONAL_ID},   // 经典CAN(8DLC)普通寻址通道.
  [1] = {.N_AI.TA = 0, .N_AI.SA = 0, .N_AI.AE = 0, .N_AI.TA_Type = 0, .AddrMode = 0, .TxDLC = 8, .FunctionalCanID = FUNCTIONAL_ID},    
  [2] = {.N_AI.TA = 0, .N_AI.SA = 0, .N_AI.AE = 0, .N_AI.TA_Type = 0, .AddrMode = 0, .TxDLC = 8, .FunctionalCanID = FUNCTIONAL_ID},
  [3] = {.N_AI.TA = 0, .N_AI.SA = 0, .N_AI.AE = 0, .N_AI.TA_Type = 0, .AddrMode = 0, .TxDLC = 8, .FunctionalCanID = FUNCTIONAL_ID}
};


// 备份数组，大小与原始表相同.
Cus_CANTP_Cfg_t ChannelConfigTable_BackUP[CHANNEL_CONFIG_TABLE_COUNT];


Cus_CANTP_Cfg_t *Cus_Cantp_GetChannel( U8 ChannalIndex )
{
  if ( ChannalIndex >= CHANNEL_CONFIG_TABLE_COUNT )  return NULL;

  return &ChannelConfigTable[ChannalIndex];
}
