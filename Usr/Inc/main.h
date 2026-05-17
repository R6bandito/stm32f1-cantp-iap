#ifndef __MAIN_H__
#define __MAIN_H__


#define __TEST__ 									(0)

#define DEVICE_ADDR								(0x12)				// 本机地址.
#define REMOTE_ADDR								(0x18)				// 对端地址.

#define RING_BUFFER_SIZE_FRAME		(32)

#include "stm32f1xx_hal.h"
#include "sys.h"
#include "debug_uart.h"
#include "CAN_Cus.h"
#include "taskInit.h"
#include "FreeRTOS.h"
#include "task.h"


#define LV_DISABLE_API_MAPPING
#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"



#endif //  __MAIN_H__
