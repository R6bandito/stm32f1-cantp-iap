#ifndef __ZET6_TASK_INIT_H__
#define __ZET6_TASK_INIT_H__


#include "FreeRTOS.h"
#include "task.h"
#include "can_tx_rx_task.h"
#include "iap_task.h"


/* --------------- CantpMainfunction ---------------------  */
  #define CANTP_MAINFUNC_TASKDEEP                   (64)
  #define CANTP_MAINFUNC_PRIORITY                   (7)
/* --------------- CantpMainfunction ---------------------  */

/* -------------------- IAP ------------------------------  */
  #define IAP_UPDATE_TASKDEEP                       (128)
  #define IAP_UPDATE_PRIORITY                       (5)
/* -------------------- IAP ------------------------------  */

/* -------------------- RxLogic ------------------------------  */
#define CANTP_RXLOGIC_TASKDEEP                       (256)
#define CANTP_RXLOGIC_PRIORITY                       (5)
/* -------------------- RxLogic ------------------------------  */


void taskCreate_CantpMainfunction( void );
void taskCreate_Cantp_RxLogic( void );
void taskCreate_IapUpdate( void );



#endif // __ZET6_TASK_INIT_H__
