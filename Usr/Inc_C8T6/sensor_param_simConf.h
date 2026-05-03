#ifndef __SENSOR_CONF_H__
#define __SENSOR_CONF_H__


/* ********************** Config ******************************* */
  #define SIM_IDLE_HOLD_TIME_MS            (10000)    // 怠速保持时间.
  #define SIM_CRUISE_HOLD_TIME_MS          (10000)   // 巡航保持时间.

  #define ENG_RPM_DELTA                    (50)     // 转速增量.
  #define ENG_RPM_IDLE                     (600)     // 引擎初始转速(怠速).
  #define ENG_RPM_LIMIT                    (8000)    // 所加速到的最大转速.

  #define VEHICLE_SPEED_START_LIM          (2500)    // 汽车起步阶段RPM阈值(模拟离合器接合).
  #define VEHICLE_SPEED_ACCELERATION_LIM   (5000)    // 汽车中段加速阶段RPM阈值(模拟降挡提速).
  #define VEHICLE_SPEED_CRUISE_LIM         (ENG_RPM_LIMIT)   // 高速巡航阶段RPM阈值(模拟最高挡位).

  #define VEHICLE_SPEED_RATIO_CREEP        (0.006f)  // 起步阶段 RPM→车速 转换系数.
  #define VEHICLE_SPEED_RATIO_ACCEL        (0.06f)   // 加速阶段 RPM→车速 转换系数.
  #define VEHICLE_SPEED_RATIO_CRUISE       (0.02f)   // 巡航阶段 RPM→车速 转换系数.

  #define COOLANT_TEMP_INIT                (25)      // 初始水温.
  #define COOLANT_TEMP_TARGET              (95)      // 目标温度(发动机工作温度). 
  #define COOLANT_TEMP_TAU                 (20)      // 热惯性系数.(值越大升温越慢)
  #define COOLANT_TEMP_JITTER              (3)       // 稳定时温度抖动幅度.(+-3)


  #define DHT11_PORT                       (GPIOA)
  #define DHT11_PIN                        (GPIO_PIN_7)
  #define DHT11_TIMEOUT_THRESHOLD          (10000)
/* ************************************************************** */


#endif // __SENSOR_CONF_H__

