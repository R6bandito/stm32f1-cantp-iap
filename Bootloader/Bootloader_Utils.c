#include "Bootloader_Utils.h"
#include <stdio.h>

#if (USE_UTILS_DEBUG)
  UART_HandleTypeDef husart;
  #if 1
    #if (__ARMCC_VERSION >= 6010050)            /* 使用AC6编译器时 */
    __asm(".global __use_no_semihosting\n\t");  /* 声明不使用半主机模式 */
    __asm(".global __ARM_use_no_argv \n\t");    /* AC6下需要声明main函数为无参数格式，否则部分例程可能出现半主机模式 */

    #else
    /* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
    #pragma import(__use_no_semihosting)
  #endif

    struct __FILE
    {
        int handle;
            
    };
    FILE __stdout;

    int _ttywrch(int ch)
    {
        ch = ch;
        return ch;
    }

    void _sys_exit(int x)
    {
        x = x;
    }

    char *_sys_command_string(char *cmd, int len)
    {
        return NULL;
    }

    /* 重定向 printf. */
    int fputc(int ch, FILE *f) {
        (void)f;
        if ( HAL_UART_Transmit(&husart, (uint8_t*)&ch, 1, 500) != HAL_OK )
        {
          /* Error Handler. */
        }
        return ch;
      }
  #endif

/* --------------------------------------------------------------------------------- */
void Cus_Bootloader_Utils_DebugUART( void );
/* --------------------------------------------------------------------------------- */

  void Cus_Bootloader_Utils_DebugUART( void )
  {
    /* Debug UART Instance CLKConfig */
    // @note: If your MCU has additional UART/USART instances (e.g., USART6, UART7, UART8, etc.),
    //       please add the corresponding clock enable statements here using the same pattern.
    //       Example:
    //       #if defined(USART6)
    //         if ( DB_UART_INSTANCE == USART6 )   __HAL_RCC_USART6_CLK_ENABLE();
    //       #endif
    if ( DB_UART_INSTANCE == USART1 )   __HAL_RCC_USART1_CLK_ENABLE();
    else if ( DB_UART_INSTANCE == USART2 )  __HAL_RCC_USART2_CLK_ENABLE();

    #if defined(USART3)
      if ( DB_UART_INSTANCE == USART3 )   __HAL_RCC_USART3_CLK_ENABLE();
    #endif // 

    #if defined(USART4)
      if ( DB_UART_INSTANCE == USART4 )   __HAL_RCC_USART4_CLK_ENABLE();
    #endif 

    #if defined(USART5)
      if ( DB_UART_INSTANCE == USART5 )   __HAL_RCC_USART5_CLK_ENABLE();
    #endif 


    /* GPIO Port CLKConfig */
    // @note: If your MCU has additional GPIO ports (e.g., GPIOG, GPIOH, GPIOI, etc.), 
    //       please add the corresponding clock enable statements here using the same pattern.
    //       Example: 
    //       #if defined(GPIOG)
    //         if ( DB_UART_GPIOPORT == GPIOG )   __HAL_RCC_GPIOG_CLK_ENABLE();
    //       #endif
    if ( DB_UART_GPIOPORT == GPIOA )   __HAL_RCC_GPIOA_CLK_ENABLE();
    else if ( DB_UART_GPIOPORT == GPIOB )    __HAL_RCC_GPIOB_CLK_ENABLE();
    else if ( DB_UART_GPIOPORT == GPIOC )    __HAL_RCC_GPIOC_CLK_ENABLE();
    else if ( DB_UART_GPIOPORT == GPIOD )    __HAL_RCC_GPIOD_CLK_ENABLE();

    #if defined(GPIOE)
      if ( DB_UART_GPIOPORT == GPIOE )   __HAL_RCC_GPIOE_CLK_ENABLE(); 
    #endif 

    #if defined(GPIOF)
      if ( DB_UART_GPIOPORT == GPIOF )   __HAL_RCC_GPIOF_CLK_ENABLE();
    #endif 


  /* GPIO_Config */
  /* Select GPIO configuration method: 0 = default (TX AF_PP, RX INPUT), 1 = user-defined. */
  #define YOUR_CUSTOM_GPIO   (0)
  #if YOUR_CUSTOM_GPIO == 0
  {
    GPIO_InitTypeDef GpioInitStructure;
    GpioInitStructure.Pin = DB_UART_TX_PIN;
    GpioInitStructure.Mode = GPIO_MODE_AF_PP;
    GpioInitStructure.Pull = GPIO_PULLUP;
    GpioInitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(DB_UART_GPIOPORT, &GpioInitStructure);

    GpioInitStructure.Pin = DB_UART_RX_PIN;
    GpioInitStructure.Mode = GPIO_MODE_INPUT;
    GpioInitStructure.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(DB_UART_GPIOPORT, &GpioInitStructure);
  }
  #elif YOUR_CUSTOM_GPIO == 1
    {
      // TODO: Add your own GPIO initialization code here.
      // e.g., HAL_GPIO_Init(...) with custom parameters.

    }
  #endif // YOUR_CUSTOM_GPIO
  #undef YOUR_CUSTOM_GPIO

  /* UART Config */
  #define YOUR_CUSTOM_UART    (0)
  #if YOUR_CUSTOM_UART == 0 
  {
    husart.Instance = DB_UART_INSTANCE;
    husart.Init.BaudRate = DB_UART_BAUDRATE;
    husart.Init.Mode = USART_MODE_TX_RX;
    husart.Init.Parity = USART_PARITY_NONE;
    husart.Init.StopBits = USART_STOPBITS_1;
    husart.Init.WordLength = USART_WORDLENGTH_8B;

    HAL_UART_Init(&husart);
  }
  #elif YOUR_CUSTOM_UART == 1
  {
    // Your Code Here...

  }
  #endif // YOUR_CUSTOM_UART
  #undef YOUR_CUSTOM_UART
  }
#endif 



#if (USE_UTILS_SYSCONF)
/**
 * @brief System clock configuration helper.
 * @note  The default implementation (YOUR_SYSCLK_CONF == 0) is designed for STM32F1 series
 *        with HSE + PLL to achieve 72 MHz. If your MCU is not STM32F1 or you need a different
 *        clock setup (e.g., use HSI, change frequency, or other families like F4/H7),
 *        please set YOUR_SYSCLK_CONF to 1 and provide your own implementation in the
 *        corresponding branch.
 */
/* --------------------------------------------------------------------------------- */
  void Cus_Bootloader_Utils_SystemClockConfig( void );
/* --------------------------------------------------------------------------------- */

  void Cus_Bootloader_Utils_SystemClockConfig( void )
  {
    #define YOUR_SYSCLK_CONF  (0)
    #if YOUR_SYSCLK_CONF == 0 
    {
      RCC_OscInitTypeDef OscInitStructure = { 0 };
      RCC_ClkInitTypeDef ClkInitStructure = { 0 };

      OscInitStructure.HSEState = RCC_HSE_ON;
      OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_HSE;
      OscInitStructure.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
      OscInitStructure.PLL.PLLState = RCC_PLL_ON;
      OscInitStructure.PLL.PLLSource = RCC_PLLSOURCE_HSE;
      OscInitStructure.PLL.PLLMUL = RCC_PLL_MUL9;

      if ( HAL_RCC_OscConfig(&OscInitStructure) != HAL_OK )
      {
        #if (USE_UTILS_DEBUG)
          printf("HAL_RCC_OscConfig Failed.\n");
        #endif
        for( ; ; );
      }

      ClkInitStructure.ClockType = RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK;
      ClkInitStructure.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
      ClkInitStructure.AHBCLKDivider = RCC_SYSCLK_DIV1;
      ClkInitStructure.APB1CLKDivider = RCC_HCLK_DIV2;
      ClkInitStructure.APB2CLKDivider = RCC_HCLK_DIV1;

      if ( HAL_RCC_ClockConfig(&ClkInitStructure, FLASH_LATENCY_2) != HAL_OK )
      {
        #if (USE_UTILS_DEBUG)
          printf("HAL_RCC_ClockConfig.\n");
        #endif
        for( ; ; );
      }
    }
    #elif YOUR_SYSCLK_CONF == 1
    {
      // @todo: Replace with your MCU-specific clock configuration.

    }
    #endif // YOUR_SYSCLK_CONF
  }

#endif // USE_UTILS_SYSCONF

