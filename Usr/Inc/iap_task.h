#ifndef __TASK_IAP_H__
#define __TASK_IAP_H__


#include "FreeRTOS.h"
#include "task.h"

/* *************************************** */
#define DOWNLOAD__ADDRESS                (0x08040000UL)
#define DOWNLOAD_FIELD_SIZE              (0x00038000UL)    // 224KB
#define IAP_MAGIC                        (0xAA55UL)
#define IAP_INFO_STRUCT_ADDR             (0x0807F800UL)
#define IAP_INFO_STRUCT_FIELD_SIZE       (0x00000800UL)    // 2KB
/* *************************************** */


void cTask_iapUpdate( void * pvParameters );


#endif // __TASK_IAP_H__
