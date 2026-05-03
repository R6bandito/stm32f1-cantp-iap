#include "sensor_param_sim.h"

/* *********** g_ver & static & handle */
TIM_HandleTypeDef htim4;
SensorData_t g_sensor_data;
static uint8_t is_curise;     // 是否处于巡航状态(稳定运行).
static uint8_t is_idle;       // 是否处于怠速状态.
/* *********************************** */


/* *********************************** */
static void DHT11_Delay_us( uint32_t us ) ;
void C8T6_Sensor_BoardParamSimInit( void );
void C8T6_Sensor_EngineRPMSimUpdate( void );
uint16_t C8T6_Sensor_EngineRPMGet( void );
uint8_t C8T6_Sensor_VehicleSpeedGet( void );
void C8T6_Sensor_VehicleSpeedSimUpdate( void );
static bool C8T6_Sensor_SimParamValidate( void );
void C8T6_Sensor_CoolantSimUpdate( void );
int8_t C8T6_Sensor_CoolantSimGet( void );

static void C8T6_Sensor_DHT11Init( void );
static void C8T6_Sensor_DHT11PINModeSwitch( uint8_t Mode );
static void C8T6_Sensor_DHT11MasterStart( void );
void C8T6_Sensor_DHT11ReceiveData( void );
int8_t C8T6_Sensor_DHT11TempGet( void );
int8_t C8T6_Sensor_DHT11HumidityGet( void );
/* *********************************** */



void C8T6_Sensor_BoardParamSimInit( void )
{
  bool is_ConfigValid = C8T6_Sensor_SimParamValidate();
  if ( !is_ConfigValid )
  {
    // 配置参数无效. 
    printf("\nSimulator Param Config Error......\n");
    for( ; ; );
  }

  C8T6_Sensor_DHT11Init();

  __HAL_RCC_TIM4_CLK_ENABLE();

  // 每 100ms 进行一次模拟采样.
  htim4.Instance = TIM4;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 100000 - 1;
  htim4.Init.Prescaler = 72 - 1;
  htim4.Init.RepetitionCounter = 0;

  if ( HAL_TIM_Base_Init(&htim4) != HAL_OK )
  {
    printf("EngineRPMSimInit Failed!\n");
    for( ; ; );
  }

  HAL_NVIC_SetPriority(TIM4_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);

  if ( HAL_TIM_Base_Start_IT(&htim4) != HAL_OK )
  {
    printf("EngineRPMSimInit Failed!\n");
    for( ; ; );
  }
}


static void DHT11_Delay_us( uint32_t us ) 
{
  uint32_t start = SysTick->VAL;
  uint32_t ticks = us * (SystemCoreClock / 1000000); // 需要的 tick 数
  while((start - SysTick->VAL) < ticks);
}


static bool C8T6_Sensor_SimParamValidate( void )
{
  // 模拟参数配置检查.
  if ( ENG_RPM_DELTA < 50 || ENG_RPM_DELTA > 500 ) 
  {
    printf("\nConfig: ENG_RPM_DELTA Error. Should between 50 and 500.\n");
    return false;
  } 

  if ( ENG_RPM_IDLE < 600 || ENG_RPM_IDLE > 1000 )
  {
    printf("\nConfig: ENG_RPM_IDLE Error. Should between 600 and 1000.\n");
    return false;
  }

  if ( ENG_RPM_LIMIT < 6000 || ENG_RPM_LIMIT > 12000 )
  {
    printf("\nConfig: ENG_RPM_LIMIT Error. Should between 6000 and 12000.\n");
    return false;
  }

  if ( VEHICLE_SPEED_START_LIM < 1500 || VEHICLE_SPEED_START_LIM > 3500 )
  {
    printf("\nConfig: VEHICLE_SPEED_START_LIM Error. Should between 1500 and 3500.\n");
    return false;
  }

  if ( VEHICLE_SPEED_ACCELERATION_LIM < 4000 || VEHICLE_SPEED_ACCELERATION_LIM > 6000 )
  {
    printf("\nConfig: VEHICLE_SPEED_ACCELERATION_LIM Error. Should between 4000 and 6000.\n");
    return false;
  }

  if ( VEHICLE_SPEED_CRUISE_LIM != ENG_RPM_LIMIT )
  {
    printf("\nConfig: VEHICLE_SPEED_CRUISE_LIM Error. VEHICLE_SPEED_CRUISE_LIM must equal to ENG_RPM_LIMIT.\n");
    return false;
  }

  if ( VEHICLE_SPEED_RATIO_CREEP < 0.003f || VEHICLE_SPEED_RATIO_CREEP > 0.01f )
  {
    printf("\nConfig: VEHICLE_SPEED_RATIO_CREEP Error. Should between 0.003 and 0.01.\n");
    return false;
  }

  if ( VEHICLE_SPEED_RATIO_ACCEL < 0.05 || VEHICLE_SPEED_RATIO_ACCEL > 0.15 )
  {
    printf("\nConfig: VEHICLE_SPEED_RATIO_ACCEL Error. Should between 0.05 and 0.15.\n");
    return false;
  }

  if ( VEHICLE_SPEED_RATIO_CRUISE < 0.01 || VEHICLE_SPEED_RATIO_CRUISE > 0.05 )
  {
    printf("\nConfig: VEHICLE_SPEED_RATIO_CRUISE Error. Should between 0.01 and 0.05.\n");
    return false;
  }

  return true;
}


/* ---------------------------------------------------------- */
void C8T6_Sensor_EngineRPMSimUpdate( void )
{
  static int16_t delta = ENG_RPM_DELTA;     // 增量.
  static uint16_t rpm = ENG_RPM_IDLE;     // 初始转速.       
  static uint32_t it_count = 0;

  if ( !is_curise && !is_idle  )
  {
    rpm += delta;
  }
  else 
  {
    if ( is_curise )
    {
      it_count++;                   // 100ms中断计数. 用于简单计算状态维持时间.
      if ( (it_count * 100) == SIM_CRUISE_HOLD_TIME_MS )  
      {
        it_count = 0;
        is_curise = 0;
      }
    }
    else if ( is_idle )
    {
      it_count++;
      if ( (it_count * 100) == SIM_IDLE_HOLD_TIME_MS )
      {
        it_count = 0;
        is_idle = 0;
      }
    }
  }

  if ( rpm >= ENG_RPM_LIMIT && is_curise != 1 )
  {
    // 转速达到上限. 折返.
    rpm = ENG_RPM_LIMIT;
    delta = -ENG_RPM_DELTA;
    is_curise = 1;              // 进入稳定状态.
  }
  else if ( rpm <= ENG_RPM_IDLE && is_idle != 1 )
  {
    // 转速达到怠速(下限).
    rpm = ENG_RPM_IDLE;
    delta = ENG_RPM_DELTA;
    is_idle = 1;                // 进入怠速状态.
  }

  int16_t vibrate = (rand() % 61) - 30;     // 模拟抖动.
  rpm += vibrate;

  if ( rpm > ENG_RPM_LIMIT )  rpm = ENG_RPM_LIMIT;
  if ( rpm < ENG_RPM_IDLE )  rpm = ENG_RPM_IDLE;

  g_sensor_data.rpm = rpm;
}


uint16_t C8T6_Sensor_EngineRPMGet( void )
{
  return g_sensor_data.rpm;
}
/* ---------------------------------------------------------- */


/* ---------------------------------------------------------- */
void C8T6_Sensor_VehicleSpeedSimUpdate( void )
{
  uint16_t engineRpm = C8T6_Sensor_EngineRPMGet();
  float vehicle_speed = 0;

  if ( engineRpm >= ENG_RPM_IDLE && engineRpm < VEHICLE_SPEED_START_LIM )
  {
  /*
    * 起步阶段（800 ~ 2500 RPM）
    * 公式：speed = (RPM - 怠速) × 起步系数
    * 例：800 RPM → 0 km/h，2500 RPM → (2500-800)×0.006 ≈ 10 km/h
  */
    vehicle_speed = (engineRpm - ENG_RPM_IDLE) * VEHICLE_SPEED_RATIO_CREEP;
  }
  else if ( engineRpm >= VEHICLE_SPEED_START_LIM && engineRpm < VEHICLE_SPEED_ACCELERATION_LIM )
  {
  /*
    * 加速阶段（2500 ~ 5000 RPM）
    * 公式：speed = 上一区间终点速度 + (RPM - 加速起点) × 加速系数
    * 上一区间终点速度 = (START_LIM - 怠速) × 蠕行系数
    *                   = (2500 - 800) × 0.006 ≈ 10 km/h
    * 例：2500 RPM → 10 km/h，5000 RPM → 10 + (5000-2500)×0.08 ≈ 210 km/h
  */
    float b = (VEHICLE_SPEED_START_LIM - ENG_RPM_IDLE) * VEHICLE_SPEED_RATIO_CREEP;
    vehicle_speed = b + (engineRpm - VEHICLE_SPEED_START_LIM) * VEHICLE_SPEED_RATIO_ACCEL;
  }
  else if ( engineRpm >= VEHICLE_SPEED_ACCELERATION_LIM && engineRpm <= VEHICLE_SPEED_CRUISE_LIM )
  {
  /*
    * 高速巡航阶段（5000 ~ 8000 RPM）
    * 公式：speed = 前两区间累积速度 + (RPM - 巡航起点) × 巡航系数
    * 
    * 累积速度 = 起步区间贡献 + 加速区间贡献
    *         = (START_LIM - 怠速) × 蠕行系数 + (ACCEL_LIM - START_LIM) × 加速系数
    *         = (2500-800)×0.006 + (5000-2500)×0.08
    *         ≈ 10 + 200 = 210 km/h
    * 
    * 例：5000 RPM → 210 km/h，8000 RPM → 210 + (8000-5000)×0.02 ≈ 270 km/h
  */
    float a = (VEHICLE_SPEED_START_LIM - ENG_RPM_IDLE) * VEHICLE_SPEED_RATIO_CREEP;
    float b = (VEHICLE_SPEED_ACCELERATION_LIM - VEHICLE_SPEED_START_LIM) * VEHICLE_SPEED_RATIO_ACCEL;
    vehicle_speed = (a + b) + (engineRpm - VEHICLE_SPEED_ACCELERATION_LIM) * VEHICLE_SPEED_RATIO_CRUISE;
  }

  // 限幅.
  if (vehicle_speed > 240.0f) vehicle_speed = 240.0f;
  if (vehicle_speed < 0.0f)   vehicle_speed = 0.0f;

  g_sensor_data.speed = (uint8_t)(vehicle_speed + 0.5f);  
}


uint8_t C8T6_Sensor_VehicleSpeedGet( void )
{
  return g_sensor_data.speed;
}
/* ---------------------------------------------------------- */


/* ---------------------------------------------------------- */
void C8T6_Sensor_CoolantSimUpdate( void )
{
  static float coolant = COOLANT_TEMP_INIT;
  const uint16_t current_rpm = C8T6_Sensor_EngineRPMGet();
  float T_target;

  float tau;   // 根据发动机工作状态微调热惯性系数.
  if ( current_rpm < VEHICLE_SPEED_START_LIM )
  {
    // 刚起步状态. 
    tau = COOLANT_TEMP_TAU * 1.2;
  }
  else 
  {
    tau = COOLANT_TEMP_TAU * 0.7;   // 中段加速状态. 发动机产热多.
  }

  if ( is_idle )
  {
    T_target = COOLANT_TEMP_INIT;   // 降温到怠速温度.
  }
  else 
  {
    T_target = COOLANT_TEMP_TARGET;
  }

  // T(t+1) = T(t) + (T_target - T(t)) / k.  (k 热惯性系数)
  coolant = coolant + (T_target - coolant) / tau; 

  float diff = T_target - coolant;
  if ( diff < 0 ) diff = -diff;     // 取绝对值. 求误差.

  if ( diff < 2.0f )    // 达到稳态后叠加随机抖动.(与T_target +- 2℃即可认为进入稳定状态.)
  {
    coolant += (int)(rand() % (COOLANT_TEMP_JITTER * 2 + 1)) - COOLANT_TEMP_JITTER;
  }

  if (coolant < -40) coolant = -40;
  if (coolant > 130)  coolant = 130;

  g_sensor_data.coolant = (int8_t)coolant;
}


int8_t C8T6_Sensor_CoolantSimGet( void )
{
  return g_sensor_data.coolant;
}
/* ---------------------------------------------------------- */


/* ---------------------------------------------------------- */
static void C8T6_Sensor_DHT11Init( void )
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef DHT11GPIO_InitStructure;

  DHT11GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;   // 配置为推挽输出.
  DHT11GPIO_InitStructure.Pin = DHT11_PIN;
  DHT11GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

  HAL_GPIO_Init(DHT11_PORT, &DHT11GPIO_InitStructure);
}


static void C8T6_Sensor_DHT11MasterStart( void )
{
  // 向DHT11发起一次数据请求.
  dht11_setHigh();      // 先拉高.
  DHT11_Delay_us(30);

  dht11_setLow();       // 拉低通讯线,准备向DHT11发送复位信号. 
  HAL_Delay(20);

  dht11_setHigh();      // 释放总线.
  DHT11_Delay_us(30);

  C8T6_Sensor_DHT11PINModeSwitch( MODE_INPUT );  // 切换为输入模式.
}


static void C8T6_Sensor_DHT11PINModeSwitch( uint8_t Mode )
{
  if ( Mode == MODE_INPUT )
  {
    // 切换到输入模式.
    GPIO_InitTypeDef Structure;

    Structure.Pin = DHT11_PIN;
    Structure.Mode = GPIO_MODE_INPUT;
    Structure.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(DHT11_PORT, &Structure);
  }
  else if ( Mode == MODE_OUTPUT )
  {
    // 切换到输出模式.
    GPIO_InitTypeDef Structure;

    Structure.Mode = GPIO_MODE_OUTPUT_OD;   
    Structure.Pin = DHT11_PIN;
    Structure.Speed = GPIO_SPEED_FREQ_HIGH;
  
    HAL_GPIO_Init(DHT11_PORT, &Structure);
  }
}


void C8T6_Sensor_DHT11ReceiveData( void )
{ 
  int8_t data_buffer[5] = { 0 };   // 5字节原始数据.
  uint32_t timeout;

  C8T6_Sensor_DHT11MasterStart();    // 发起一个请求. 此时DHT11对应的引脚已被配置为输入模式.

  __disable_irq();    // 为保证时序，关掉中断.

  timeout = DHT11_TIMEOUT_THRESHOLD;
  while( dht11_Read() == GPIO_PIN_SET && timeout )  timeout--;   // 等待DHT11回应低电平.
  if ( timeout == 0 )
  {
    __enable_irq();
    C8T6_Sensor_DHT11PINModeSwitch(MODE_OUTPUT);  // 恢复输出
    return;
  }

  timeout = DHT11_TIMEOUT_THRESHOLD;
  while( dht11_Read() == GPIO_PIN_RESET && timeout )  timeout--;   // 等待DHT11再将其拉高.
  if ( timeout == 0 )
  {
    __enable_irq();
    C8T6_Sensor_DHT11PINModeSwitch(MODE_OUTPUT);  
    return;
  }

  // 接收40位数据.
  for( uint8_t i = 0; i < 5; i++ )
  {
    uint8_t byte = 0;

    for(uint8_t j = 0; j < 8; j++ )
    {
      // 等待低电平开始（确保新 bit 开始）
      timeout = DHT11_TIMEOUT_THRESHOLD;
      while( dht11_Read() == GPIO_PIN_SET && timeout )   timeout--;  

      // 等待低电平结束（约 50µs）
      timeout = DHT11_TIMEOUT_THRESHOLD;
      while( dht11_Read() == GPIO_PIN_RESET && timeout ) timeout--;

      uint32_t highTime = 0;
      while(dht11_Read() == GPIO_PIN_SET && highTime < 100)
      {
        highTime++;
        __nop();
      }

      byte <<= 1;

      // 高电平长（约 70μs）表示 1，短（约 26~28μs）表示 0.
      if ( highTime > 64 )
      {
        byte |= 0x01;   // 高电平. 
      }

    }

    data_buffer[i] = byte;
  }

  // 结束时序：等待DHT11拉低总线
  timeout = DHT11_TIMEOUT_THRESHOLD;
  while( dht11_Read() == GPIO_PIN_SET && timeout ) timeout--;
  DHT11_Delay_us(55);

  C8T6_Sensor_DHT11PINModeSwitch(MODE_OUTPUT);
  dht11_setHigh();    // 接收完后主动拉高. 空闲状态.

  __enable_irq();

  uint16_t CheckSum = data_buffer[0] + data_buffer[1] + data_buffer[2] + data_buffer[3];
  if ( CheckSum != data_buffer[4] )
  {
    // 校验和验证不正确，数据接收错误.
    printf("\nDHT11 Data Error.\n");
    C8T6_Sensor_DHT11PINModeSwitch( MODE_OUTPUT );
    return;
  }

  // 由于DHT11精度有限，此处忽略小数部分, 只取整数.
  g_sensor_data.humidity = data_buffer[0];      // 湿度整数.
  g_sensor_data.temperature = data_buffer[2];   // 温度整数.
}


int8_t C8T6_Sensor_DHT11TempGet( void )
{
  return g_sensor_data.temperature;
}


int8_t C8T6_Sensor_DHT11HumidityGet( void )
{
  return g_sensor_data.humidity;
}
/* ---------------------------------------------------------- */
