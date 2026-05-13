#include "c8t6_main.h"


/* ************** g_vrer & static ******************** */
volatile bool dht11_sample_flag = false;
volatile bool data_send_sample_flag = false;
volatile bool iap_start_flag = false;
uint8_t iap_commandBuff[6];
Cus_CANTp_Conn_t *pConn;

uint8_t initBuff[128];
uint8_t filterBuff[128];

extern UART_HandleTypeDef huart2;
/* *************************************************** */

/* *************************************************** */
  static void MX_CAN_Init( void );
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

  MX_CAN_Init();

  /* 初始化 CANTP. 创建发送连接 */
  Cus_Cantp_SystemInit();
  pConn = Cus_Cantp_CreateTxConnection(0x12, 0x18, CANTP_ADDR_MODE_COMMON, (void *)CAN1, Cus_CanTP_canSendFunc_Asynchronous, NULL);
  if ( !pConn )
  {
    printf("Cantp create TxConnection Failed.\r\n");
    for( ; ; );
  }

  C8T6_Sensor_BoardParamSimInit();

  Ymodem_UARTInit();

  __Enable_ymodem_uart_IT();

  while(1)
  {
    HAL_Delay(1000);

    if ( iap_start_flag )
    {
      __Disable_ymodem_uart_IT();   // 进入Ymodem前一定要确保接收中断关闭!

      int32_t size = Ymodem_Receive();
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
        uint8_t buffer[3] = { 0 };
        buffer[0] = SENSOR_TYPE_RPM;    // 第一个字节: 数据类型标识.
        buffer[1] = (C8T6_Sensor_EngineRPMGet() & 0xFF);    // 低 8 位.
        buffer[2] = (C8T6_Sensor_EngineRPMGet() >> 8);      // 高 8 位.
        Cus_Cantp_startTransmit(pConn, buffer, sizeof(buffer));
      }

      HAL_Delay(5);

      {
        // 发送Speed.
        uint8_t buffer[2] = { 0 };
        buffer[0] = SENSOR_TYPE_SPD;      // 速度类型数据标识.
        buffer[1] = C8T6_Sensor_VehicleSpeedGet();
        Cus_Cantp_startTransmit(pConn, buffer, sizeof(buffer));
      }

      HAL_Delay(5);

      {
        // 发送 Temp 和 Humidity.
        int8_t buffer[3] = { 0 };
        buffer[0] = SENSOR_TYPE_HUM_TEMP;     // 温湿度数据标识.
        buffer[1] = C8T6_Sensor_DHT11TempGet();
        buffer[2] = C8T6_Sensor_DHT11HumidityGet();
        Cus_Cantp_startTransmit(pConn, buffer, sizeof(buffer));
      }

      HAL_Delay(5);

      {
        int8_t buffer[2] = { 0 };
        buffer[0] = SENSOR_TYPE_COOLANT;
        buffer[1] = C8T6_Sensor_CoolantSimGet();
        Cus_Cantp_startTransmit(pConn, buffer, sizeof(buffer));
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


static void MX_CAN_Init( void )
{
  int8_t hReturn = Factory_CANInitConfig_t_Static(initBuff, sizeof(initBuff));
  if ( hReturn < 0 )
  {
    switch (hReturn)
    {
      case -1:  printf("Wrong param of Factory_CANInitConfig_t_Static.\r\n");   break;
      case -2:  printf("Buffer too small of Factory_CANInitConfig_t_Static.\r\n");   break;
      default:break;
    }
    for( ; ; );
  }

  int8_t mReturn = Factory_CANFilterConfig_t_Static(filterBuff, sizeof(filterBuff));
  if ( mReturn < 0 )
  {
    switch (mReturn)
    {
      case -1:  printf("Wrong param of Factory_CANFilterConfig_t_Static.\r\n");   break;
      case -2:  printf("Buffer too small of Factory_CANFilterConfig_t_Static.\r\n");   break;
      default:break;
    }
    for( ; ; );    
  }

  CANInitConfig_t *pInit = (CANInitConfig_t *)initBuff;
  CANFilterConfig_t *pFilter = (CANFilterConfig_t *)filterBuff;
  Cus_CAN_GPIO_t gpio = { .Alternate = 0, .CAN_GPIO_RX = GPIO_PIN_11, .CAN_GPIO_TX = GPIO_PIN_12, .CAN_GPIOPort_x = GPIOA };

  pInit->baudrate = CAN_BAUDRATE_500K;
  pInit->CAN_gpio = gpio;
  pInit->Instance = CAN1;
  pInit->Mode = MODE_NORMAL;
  pInit->SJW = Cus_CAN_SJW_1Tq;
  pInit->is_AutoBusOff = false;
  pInit->is_AutoRestransmission = false;
  pInit->is_AutoWakeUP = false;
  pInit->is_ReceiveFifoLocked = false;
  pInit->is_TimeTriggeredMode = false;
  pInit->is_TransmitFifoPriority = false;

  if ( pInit->Cus_CAN_Init(pInit) != HAL_OK )
  {
    printf("Cus_CAN_Init Failed!\r\n");
    for( ; ; );
  }

  pFilter->FIFOAssignment = Cus_CAN_FIFOASSIGNMENT_FIFO0;
  pFilter->FilterBank = 0;
  pFilter->Mode = Cus_CAN_FILTERMODE_IDLIST;
  pFilter->Scale = Cus_CAN_SCALE_32BIT;
  pFilter->is_Activation = Cus_FILTER_Enable;

  Cus_CAN_Filter_SetStdList32(pFilter, CAN_FILTER_RTR_NONE, 0x18, 0x17);      // 本机地址  0x18.

  if ( pFilter->Cus_CAN_FilterInit(pFilter, CAN1) != HAL_OK )
  {
    printf("Cus_CAN_FilterInit Failed!\r\n");
    for( ; ; );
  }

  if ( Cus_CAN_Start(CAN1) != HAL_OK )
  {
    printf("Cus_CAN_Start Failed!\r\n");
    for( ; ; );
  }

  Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
  if ( !pDev )
  {
    printf("Cus_CAN_getControlBlock Failed!\r\n");
    for( ; ; );
  }

  pDev->EnableInterrupt(pDev, CAN_IT_TX_MAILBOX_EMPTY);
  pDev->EnableInterrupt(pDev, CAN_IT_RX_FIFO0_MSG_PENDING);
}

