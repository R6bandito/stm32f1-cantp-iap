#include "main.h"

/* ------------------------- 全局配置 ----------------------------- */
	static uint8_t canStructureInitialBuffer[64];
	static uint8_t canFilterInitialBuffer[64];
	static Cus_CAN_RxMsg_t ringRecvBuffer[RING_BUFFER_SIZE_FRAME]; 
	uint8_t Cantp_RecvBuffer[HARDWARE_RECV_BUF_SIZE];

	Cus_CANTp_Conn_t *pConn_Rx1;
	Cus_CANTp_Conn_t *pConn_Tx1;
/* --------------------------------------------------------------- */

/* ------------------------- 时基分离 ----------------------------- */
	TIM_HandleTypeDef htim6;
	HAL_StatusTypeDef HAL_InitTick( uint32_t TickPriority );
/* --------------------------------------------------------------- */

/* ------------------------- 内部API ----------------------------- */
	static void MX_CAN_Init( void );
	static void Enable_CAN_IT( void );
	static void Register_CAN_RecvBuffer( void );
	static void CreateRxConnection( void );
	static void CreateTxConnection( void );
/* --------------------------------------------------------------- */

/* ------------------------- TEST -------------------------------- */
#if (__TEST__)
	static void __CAN_SELF_TEST( void );
	static void __CAN_LOOP_TEST_SINGLE_FRAME( void );
	static void __CAN_TEST_GET_INFO( void );
	static void __CAN_LOOP_TEST_MULTI_FRAMES( uint8_t frameCount );

	#if (USE_SEND_ASYNC)
		static void __CAN_TEST_ASYNC_SEND( void );
	#endif
#endif // __TEST__
/* --------------------------------------------------------------- */


int main( void )
{
	/* Bootloader 中，跳转时已经关闭中断，此处重新打开. */
	__enable_irq();

	/* 再次设置中断向量表偏移兜底 */
	SCB->VTOR = 0x08008000UL;

  HAL_Init();

  SystemClock_Config_72Mhz();

  debug_uart_Init();

	printf("\n\nZET6 Project Start Running .... \n\n");

	/* 配置bxCAN 启动相关中断 注册接收缓冲区 */
	MX_CAN_Init();
	Enable_CAN_IT();
	Register_CAN_RecvBuffer();

	/* 若开启测试选项 则执行功能自检 */
	#if (__TEST__)
		__CAN_SELF_TEST();
	#endif 

	/* 初始化CANTP系统，创建发送接收连接 */
	Cus_Cantp_SystemInit();
	CreateRxConnection();
	CreateTxConnection();

	/* 创建线程并启动调度器. (__TEST__模式下将不创建) */
  #if (__TEST__) == 0
		taskCreate_CantpMainfunction();
		taskCreate_Cantp_RxLogic();
		taskCreate_IapUpdate();

		vTaskStartScheduler();
	#endif 
	
  while(1)
  {
		/* 非 __TEST__ 模式下，程序不会运行到此处 */
		HAL_Delay(1);
  }
}


HAL_StatusTypeDef HAL_InitTick( uint32_t TickPriority )
{
	/* 重定义 HAL_InitTick ，进行时基分离. Systick交由FreeRTOS配置. */
	/* 初始化基本定时器 TIM6 作为HAL库的tick时基基准. */
	__HAL_RCC_TIM6_CLK_ENABLE();
	
	htim6.Instance = TIM6;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;

	uint32_t pclk1_freq = HAL_RCC_GetPCLK1Freq();
	uint32_t presc = (2 * pclk1_freq / 1000000 == 0) ? (2 * pclk1_freq / 100000) : (2 * pclk1_freq / 1000000);

	// Period = (1MHz * 0.001s) - 1 = 1000 - 1 = 999. (0.001s = 1ms)
	htim6.Init.Period = 1000 - 1;
	htim6.Init.Prescaler = presc - 1;
	htim6.Init.RepetitionCounter = 0;

	if ( HAL_TIM_Base_Init(&htim6) != HAL_OK )
	{
		for( ; ; );
	}

	HAL_NVIC_EnableIRQ(TIM6_IRQn);
	HAL_NVIC_SetPriority(TIM6_IRQn, TickPriority, 0);

	HAL_TIM_Base_Start_IT(&htim6);
	return HAL_OK;
}


static void MX_CAN_Init( void )
{
	int8_t hReturn = Factory_CANInitConfig_t_Static(canStructureInitialBuffer, sizeof(canStructureInitialBuffer));
	if ( hReturn < 0 )
	{
		printf("Factory_CANInitConfig_t_Static return Error.\r\n");
		for( ; ; );
	}

	hReturn = Factory_CANFilterConfig_t_Static(canFilterInitialBuffer, sizeof(canFilterInitialBuffer));
	if ( hReturn < 0 )
	{
		printf("Factory_CANFilterConfig_t_Static return Error.\r\n");
		for( ; ; );
	}

	CANInitConfig_t *pInit = (CANInitConfig_t *)canStructureInitialBuffer;
	CANFilterConfig_t *pFilter = (CANFilterConfig_t *)canFilterInitialBuffer;
	Cus_CAN_GPIO_t gpio = { .Alternate = 0, .CAN_GPIO_RX = GPIO_PIN_11, .CAN_GPIO_TX = GPIO_PIN_12, .CAN_GPIOPort_x = GPIOA };

	pInit->baudrate = CAN_BAUDRATE_500K;
	pInit->CAN_gpio = gpio;
	pInit->Instance = CAN1;

	#if (__TEST__)
		pInit->Mode = MODE_LOOPBACK;
	#else
		pInit->Mode = MODE_NORMAL;
	#endif 
	
	pInit->SJW = Cus_CAN_SJW_1Tq;
	pInit->is_AutoBusOff = false;
	pInit->is_AutoRestransmission = false;
	pInit->is_AutoWakeUP = false;
	pInit->is_ReceiveFifoLocked = false;
	pInit->is_TimeTriggeredMode = false;
	pInit->is_TransmitFifoPriority = false;

	if ( pInit->Cus_CAN_Init(pInit) != HAL_OK )
	{
		printf("pInit->Cus_CAN_Init return Error.\r\n");
		for( ; ; );
	}

	pFilter->FIFOAssignment = Cus_CAN_FIFOASSIGNMENT_FIFO0;
	pFilter->FilterBank = 0;
	pFilter->Mode = Cus_CAN_FILTERMODE_IDLIST;
	pFilter->Scale = Cus_CAN_SCALE_32BIT;
	pFilter->is_Activation = Cus_FILTER_Enable;

	Cus_CAN_Filter_SetStdList32(pFilter, CAN_FILTER_RTR_NONE, 0, 0);			// 暂时全通放行所有帧.

	if ( pFilter->Cus_CAN_FilterInit(pFilter, CAN1) != HAL_OK )
	{
		printf("pFilter->Cus_CAN_FilterInit return Error.\r\n");
		for( ; ; );
	}

	if ( Cus_CAN_Start(CAN1) != HAL_OK )
	{
		printf("Cus_CAN_Start return Error.\r\n");
		for( ; ; );
	}
}


static void Enable_CAN_IT( void )
{
	Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
	if ( !pDev )	
	{
		printf("Cus_CAN_getControlBlock return Error in Enable_CAN_IT.\r\n");
		for( ; ; );
	}

	pDev->EnableInterrupt(pDev, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY);
}


static void Register_CAN_RecvBuffer( void )
{
	Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
	if ( !pDev )
	{
		printf("Cus_CAN_getControlBlock return Error in Register_CAN_RecvBuffer.\r\n");
		for( ; ; );
	}

	if ( pDev->registerRxBuffer(pDev, (void *)ringRecvBuffer, sizeof(ringRecvBuffer)) != 0 )
	{
		printf("pDev->registerRxBuffer return Error in Register_CAN_RecvBuffer.\r\n");
		for( ; ; );
	}
}


static void CreateRxConnection( void )
{
	pConn_Rx1 = Cus_Cantp_CreateRxConnection( DEVICE_ADDR, 
																						CANTP_ADDR_MODE_COMMON, 
																						0, 5, 
																						CAN1, Cantp_RecvBuffer, 
																						sizeof(Cantp_RecvBuffer), 
																						Cus_CanTP_canSendFunc_Asynchronous, 
																						ZET6_CantpDataIndication, 
																						NULL );
	if ( !pConn_Rx1 )
	{
		printf("Cus_Cantp_CreateRxConnection return Error in CreateRxConnection.\r\n");
		for( ; ; );
	}
}


static void CreateTxConnection( void )
{
	pConn_Tx1 = Cus_Cantp_CreateTxConnection( REMOTE_ADDR, 
																						DEVICE_ADDR, 
																						CANTP_ADDR_MODE_COMMON, 
																						CAN1, 
																						Cus_CanTP_canSendFunc_Asynchronous, 
																						NULL );
	if ( !pConn_Tx1 )
	{
		printf("Cus_Cantp_CreateTxConnection return Error in CreateTxConnection.\r\n");
		return;
	}
}


/* ************************ TEST *************************** */
#if (__TEST__)
	/* 注意: 启用测试后，请务必确保 CAN_Cus.h 中关闭了 CANTP. 否则测试将会出错. */

	static void __CAN_SELF_TEST( void )
	{
		__CAN_TEST_GET_INFO();

		__CAN_LOOP_TEST_SINGLE_FRAME();

		__CAN_LOOP_TEST_MULTI_FRAMES(RING_BUFFER_SIZE_FRAME);

		#if (USE_SEND_ASYNC)
			__CAN_TEST_ASYNC_SEND();
		#endif 
	}


	static void __CAN_LOOP_TEST_SINGLE_FRAME( void )
	{
		printf("*********** __CAN_LOOP_TEST_SINGLE_FRAME START ************\n");
		Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
		if ( !pDev )	return;

		CAN_TxHeaderTypeDef TxHeader;
		TxHeader.DLC = 1;
		TxHeader.IDE = CAN_ID_STD;
		TxHeader.RTR = CAN_RTR_DATA;
		TxHeader.StdId = DEVICE_ADDR;

		uint8_t data[1] = 0xFF;

		if ( pDev->Send(pDev, TxHeader, data) != HAL_OK )
		{
			printf(" [__CAN_LOOP_TEST_SINGLE_FRAME] Failed. pDev->Send return Error.\r\n");
			printf("*********** __CAN_LOOP_TEST_SINGLE_FRAME FAILED ************\n\n");
			return;
		}

		HAL_Delay(2);

		CAN_RxHeaderTypeDef RxHeader;
		uint8_t rxD[2];
		if ( pDev->Receive_IT(pDev, &RxHeader, rxD) != HAL_OK )
		{
			printf(" [__CAN_LOOP_TEST_SINGLE_FRAME] Failed. pDev->Receive_IT return Error.\r\n");
			printf("*********** __CAN_LOOP_TEST_SINGLE_FRAME FAILED ************\n");
			return;
		}

		if ( memcmp(data, rxD, 1) == 0 )
		{
			printf("\tRecv CANID: %d\n\tRecv Data %d\n", RxHeader.StdId, rxD[0]);
			printf("*********** __CAN_LOOP_TEST_SINGLE_FRAME PASSED ************\n");
			return;
		}

		printf("*********** __CAN_LOOP_TEST_SINGLE_FRAME FAILED ************\n\n");
	}


	static void __CAN_TEST_GET_INFO( void )
	{
		printf("*********** __CAN_TEST_GET_INFO START ************\n");
		Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
		if ( !pDev )
		{
			printf(" [__CAN_TEST_GET_INFO] Failed. Cus_CAN_getControlBlock return Error.\r\n");
			printf("*********** __CAN_TEST_GET_INFO FAILED ************\n");
			return;
		}

		Cus_CAN_RateInfo_t can_info = { 0 };
		HAL_StatusTypeDef hReturn = Cus_CAN_getRateInfo(CAN1, &can_info);
		if ( hReturn != HAL_OK )
		{
			printf(" [__CAN_TEST_GET_INFO] Failed. Cus_CAN_getRateInfo return Error.\r\n");
			printf("*********** __CAN_TEST_GET_INFO FAILED ************\n");
			return;
		}
		
		printf("\tBS1: %d\n\tBS2: %d\n\tCLOCK: %d\n\tPRESCALER: %d\n\tBAUDRATE: %.1f\n", can_info.bs1, can_info.bs2, can_info.can_clock, can_info.prescaler, can_info.real_baudrate);
		printf("*********** __CAN_TEST_GET_INFO PASSED ************\n\n");
	}


	static void __CAN_LOOP_TEST_MULTI_FRAMES( uint8_t frameCount )
	{
		printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES START ************\n");
		if ( frameCount > RING_BUFFER_SIZE_FRAME )
		{
			printf(" [__CAN_LOOP_TEST_MULTI_FRAMES] Failed. Args: frameCount too large.\r\n");
			printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES FAILED ************\n");
			return;
		}

		Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
		if ( !pDev )
		{
			printf(" [__CAN_LOOP_TEST_MULTI_FRAMES] Failed. Cus_CAN_getControlBlock return Error.\r\n");
			printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES FAILED ************\n");
			return;
		}

		// 准备发送数据：每帧数据内容 = 帧序号
    for (uint8_t i = 0; i < frameCount; i++) 
		{
			CAN_TxHeaderTypeDef txHeader = {
					.DLC = 1,
					.IDE = CAN_ID_STD,
					.RTR = CAN_RTR_DATA,
					.StdId = DEVICE_ADDR
			};

			uint8_t data = i;
			if ( pDev->Send(pDev, txHeader, &data) != HAL_OK ) 
			{
				printf("Send frame %d failed\n", i);
				printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES FAILED ************\n");
				return;
			}

			HAL_Delay(1);
		}

		// 读取所有帧并校验
		uint8_t recvCount = 0;
		uint8_t rxData[1];
		while ( pDev->Receive_IT(pDev, &(CAN_RxHeaderTypeDef){0}, rxData) == HAL_OK ) 
		{
			// 检查数据是否与发送的序号匹配
			if ( rxData[0] == recvCount ) 
			{
					recvCount++;
			} 
			else 
			{
				printf("Data mismatch: expected %d, got %d\n", recvCount, rxData[0]);
				printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES FAILED ************\n");
				return;
			}
		}

		if ( recvCount == frameCount - 1 )
		{
			printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES PASSED ************\n\n");
			return;
		}

		printf("*********** __CAN_LOOP_TEST_MULTI_FRAMES FAILED ************\n");
	}


	#if (USE_SEND_ASYNC)
		static void __CAN_TEST_ASYNC_SEND( void )
		{
			printf("*********** __CAN_TEST_ASYNC_SEND START ************\n");
			Cus_CAN_Device_t *pDev = Cus_CAN_getControlBlock(CAN1);
			if ( !pDev )
			{
				printf(" [__CAN_TEST_ASYNC_SEND] Failed. Cus_CAN_getControlBlock return Error.\r\n");
				printf("*********** __CAN_TEST_ASYNC_SEND FAILED ************\n");
				return;
			}

			if ( !pDev->CheckInterrupt(pDev, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY) )
			{
				printf(" [__CAN_TEST_ASYNC_SEND] Failed. pDev->CheckInterrupt return Error.\r\n");
				printf("*********** __CAN_TEST_ASYNC_SEND FAILED ************\n");
				return;
			}

			CAN_TxHeaderTypeDef txHeader = {
				.DLC = 1,
				.IDE = CAN_ID_STD,
				.RTR = CAN_RTR_DATA,
				.StdId = DEVICE_ADDR
			};
			for (int i = 0; i < TX_NODE_POLL_SIZE; i++) 
			{
				uint8_t data = 1 + i;

				/* 连续快速发送多帧 */
        pDev->Send_IT(pDev, txHeader, data);
    	}

			HAL_Delay(2);

			CAN_RxHeaderTypeDef RxHeader = { 0 };
			uint8_t rxData[1] = { 0 };
			uint8_t recvCount = 0;
			while ( pDev->Receive_IT(pDev, &RxHeader, rxData) == HAL_OK ) 
			{
				recvCount++;

				printf("\tFrame: %d\n\tDLC: %d\n\tID: %d\n\tData: %d\n\n\n", recvCount, RxHeader.DLC, RxHeader.StdId, rxData[0]);
			}

			if ( recvCount == TX_NODE_POLL_SIZE )
			{
				printf("*********** __CAN_TEST_ASYNC_SEND PASSED ************\n\n");
				return;
			}

			printf("*********** __CAN_TEST_ASYNC_SEND FAILED ************\n");
		}
	#endif 

#endif // static void __CAN_LOOP_TEST_SINGLE_FRAME( void )
/* ********************************************************* */
	
	