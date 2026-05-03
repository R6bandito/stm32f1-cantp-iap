#include "c8t6_main.h"


/* ************** g_vrer & static ******************** */
volatile bool dht11_sample_flag = false;
volatile bool data_send_sample_flag = false;
volatile bool iap_start_flag = false;
uint8_t iap_commandBuff[8];

extern UART_HandleTypeDef huart2;
/* *************************************************** */


/* *************************************************** */

  // 开启 Ymodem-UART 的接收中断. (注意！由于Ymodem实现是基于轮询串口实现的，因此进入Ymodem逻辑时，必须保证IT关闭).
  // 此处用于接收IAP命令.
  void __Enable_ymodem_uart_IT( void );     

  void __Disable_ymodem_uart_IT( void );

/* *************************************************** */


int main( void )
{
  HAL_Init();

  SystemClock_Config_72Mhz();

  debug_uart_Init();

  printf("C8T6 Running.\nSystemClock: %d.\n", SystemCoreClock);

  C8T6_Sensor_BoardParamSimInit();

  Ymodem_UARTInit();

  __Enable_ymodem_uart_IT();

  while(1)
  {
    HAL_Delay(1000);

    if ( iap_start_flag )
    {
      __Disable_ymodem_uart_IT();   // 进入Ymodem前一定要确保接收中断关闭!

      int8_t size = Ymodem_Receive();
      if ( size <= 0 )
      {
        printf("\nYmodem_Receive Failed!\n");
      }

      iap_start_flag = false;

      __Enable_ymodem_uart_IT();    // 一次Ymodem通信已经完成(无论成功与否). 重新开启中断等待下一次命令.
    }

    if ( dht11_sample_flag )
    {
      // 每 2s 向DHT11获取一次数据.
      C8T6_Sensor_DHT11ReceiveData();
      dht11_sample_flag = false;
    }

    if ( data_send_sample_flag )
    {
      // 每 500ms 向对端发送一次报文.
      {
        // 发送RPM. (单帧发送)
        uint8_t buffer[6] = { 0 };
        buffer[0] = SENSOR_TYPE_RPM;    // 第一个字节: 数据类型标识.
        buffer[1] = (C8T6_Sensor_EngineRPMGet() & 0xFF);    // 低 8 位.
        buffer[2] = (C8T6_Sensor_EngineRPMGet() >> 8);      // 高 8 位.
        Cus_Cantp_Transmit(0, 1, buffer, sizeof(buffer), CAN1);   // 无需检测返回值. 若请求失败继续. 等待下一次发送.
      }

      {
        // 发送Speed.
        uint8_t buffer[6] = { 0 };
        buffer[0] = SENSOR_TYPE_SPD;      // 速度类型数据标识.
        buffer[1] = C8T6_Sensor_VehicleSpeedGet();
        Cus_Cantp_Transmit(0, 1, buffer, sizeof(buffer), CAN1);
      }

      {
        // 发送 Temp 和 Humidity.
        int8_t buffer[6] = { 0 };
        buffer[0] = SENSOR_TYPE_HUM_TEMP;     // 温湿度数据标识.
        buffer[1] = C8T6_Sensor_DHT11TempGet();
        buffer[2] = C8T6_Sensor_DHT11HumidityGet();
        Cus_Cantp_Transmit(0, 1, buffer, sizeof(buffer), CAN1);
      }

      {
        int8_t buffer[6] = { 0 };
        buffer[0] = SENSOR_TYPE_COOLANT;
        buffer[1] = C8T6_Sensor_CoolantSimGet();
        Cus_Cantp_Transmit(0, 1, buffer, sizeof(buffer), CAN1);
      }

      data_send_sample_flag = false;
    }

    printf("temperature: %d\nhumidity: %d\n", C8T6_Sensor_DHT11TempGet(), C8T6_Sensor_DHT11HumidityGet());
    printf("RPM: %d\nSPD: %d\nCoolant: %d\n", C8T6_Sensor_EngineRPMGet(), C8T6_Sensor_VehicleSpeedGet(), C8T6_Sensor_CoolantSimGet());
  }
}


void __Enable_ymodem_uart_IT( void )
{
  HAL_UART_AbortReceive(&huart2);   // 确保重新开始前是干净的(重置 UART 的接收状态).

  __HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_RXNE | UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_NE);  

  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);    // 优先级比定时器刷新数据高.

  HAL_NVIC_EnableIRQ(USART2_IRQn);

  HAL_UART_Receive_IT(&huart2, iap_commandBuff, sizeof(iap_commandBuff));
}


void __Disable_ymodem_uart_IT( void )
{
  HAL_UART_AbortReceive(&huart2);

  __HAL_UART_DISABLE_IT(&huart2, UART_IT_RXNE);

  __HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_RXNE | UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_NE);  

  HAL_NVIC_DisableIRQ(USART2_IRQn);

  HAL_NVIC_ClearPendingIRQ(USART2_IRQn);
}

