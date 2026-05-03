#ifndef __SENSOR_SIM_H__
#define __SENSOR_SIM_H__


#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sensor_param_simConf.h"
#include "sensor_data.h"


#define MODE_OUTPUT          (0xFF)
#define MODE_INPUT           (0)

#define dht11_setHigh()      HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET)
#define dht11_setLow()       HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET)
#define dht11_Read()         HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)

/* ****************************** */
void C8T6_Sensor_BoardParamSimInit( void );

void C8T6_Sensor_EngineRPMSimUpdate( void );
uint16_t C8T6_Sensor_EngineRPMGet( void );

void C8T6_Sensor_VehicleSpeedSimUpdate( void );
uint8_t C8T6_Sensor_VehicleSpeedGet( void );

void C8T6_Sensor_CoolantSimUpdate( void );
int8_t C8T6_Sensor_CoolantSimGet( void );

void C8T6_Sensor_DHT11ReceiveData( void );
int8_t C8T6_Sensor_DHT11TempGet( void );
int8_t C8T6_Sensor_DHT11HumidityGet( void );
/* ****************************** */

#endif // __SENSOR_SIM_H__
