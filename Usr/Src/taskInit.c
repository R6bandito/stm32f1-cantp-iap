#include "taskInit.h"


/* ****************** TaskHandle ******************** */
TaskHandle_t Cantp_Mainfunc_Taskhandle;
TaskHandle_t Iap_Update_Taskhandle;
TaskHandle_t Cantp_RxLogic_Taskhandle;
/* ****************** TaskHandle ******************** */


void taskCreate_CantpMainfunction( void )
{
  BaseType_t cReturn = xTaskCreate( cTask_cantpMainFunction,
                                    "CantpMain", 
                                    CANTP_MAINFUNC_TASKDEEP, 
                                    NULL, 
                                    CANTP_MAINFUNC_PRIORITY, 
                                    &Cantp_Mainfunc_Taskhandle );
  if ( cReturn != pdPASS )
  {
    printf("[Task Create Failed: cTask_cantpMainFunction]\r\n");
    for( ; ; );
  }
}


void taskCreate_IapUpdate( void )
{
  BaseType_t cReturn = xTaskCreate( cTask_iapUpdate, 
                                    "IapUpdate", 
                                    IAP_UPDATE_TASKDEEP, 
                                    NULL, 
                                    IAP_UPDATE_PRIORITY, 
                                    &Iap_Update_Taskhandle );
  if ( cReturn != pdPASS )
  {
    printf("[Task Create Failed: cTask_iapUpdate]\r\n");
    for( ; ; );
  }
}


void taskCreate_Cantp_RxLogic( void )
{
  BaseType_t cReturn = xTaskCreate( cTask_cantpRxFunction, 
                                    "RxLogic", 
                                    CANTP_RXLOGIC_TASKDEEP, 
                                    NULL, 
                                    CANTP_RXLOGIC_PRIORITY, 
                                    &Cantp_RxLogic_Taskhandle );
  if ( cReturn != pdPASS )
  {
    printf("[Task Create Failed: taskCreate_Cantp_RxLogic]\r\n");
    for( ; ; );
  }
}


