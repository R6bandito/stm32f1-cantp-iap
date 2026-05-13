#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#include <stdint.h>

/* *************************************** */
typedef enum 
{
  SENSOR_TYPE_RPM           = 0x01,               // 发动机转速 参数.
  SENSOR_TYPE_SPD           = 0x02,               // 车速 参数.
  SENSOR_TYPE_HUM_TEMP      = 0x03,                // 温度湿度 参数.  
  SENSOR_TYPE_COOLANT       = 0x04               // 冷却液温度 参数.
    
} SensorType_t;
/* *************************************** */


/* *************************************** */
typedef struct 
{
  uint16_t rpm;
  uint8_t speed;
  int8_t coolant;
  int8_t temperature;
  int8_t humidity;

} SensorData_t;
/* *************************************** */


#endif // __SENSOR_DATA_H__

