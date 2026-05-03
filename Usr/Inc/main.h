#ifndef __MAIN_H__
#define __MAIN_H__


#include "stm32f1xx_hal.h"
#include "sys.h"
#include "debug_uart.h"
#include "ds.h"


#if (STM32F103ZET6)
	#include "FreeRTOS.h"
	#include "task.h"
#endif // STM32F103ZET6

#if (STM32F103ZET6)
	#define LV_DISABLE_API_MAPPING
	#define LV_CONF_INCLUDE_SIMPLE
	#include "lvgl.h"
#endif // STM32F103ZET6



#endif //  __MAIN_H__
