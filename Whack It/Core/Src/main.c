#include "main.h"
#include "lcd.h"
#include "stdio.h"

/* Private variables ---------------------------------------------------------*/
static int gameStart = 0;
static uint32_t random_LED = 99;
static int score = 0;
int gameOver = 0;
char a[20];
char display[7] = {'S', 'c', 'o', 'r', 'e', ':', ' '};
char displayScore[27];

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM5_Init(void);
void delay3sec(void);
void delay1sec(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void TIM2_IRQHandler(void);
void TRNG_Init(void);
uint32_t TRNG_GetRandomNumber(uint32_t min, uint32_t max);
void updateScore(void);
void startGame(void);

void delay1sec(void)
{
	TIM2->CNT = 0;
    uint32_t start = TIM2->CNT;
    random_LED = TRNG_GetRandomNumber(0, 5);
    GPIOC->ODR = 0;
    GPIOC->ODR |= (uint32_t)(1<<random_LED);
    while ((TIM2->CNT - start) < TIM2->ARR);
}

void delay3sec(void)
{
	TIM5->CNT = 0;
	uint32_t start = TIM5->CNT;
	GPIOD->ODR ^= (1<<12);
	gameStart = 1;
	while ((TIM5->CNT - start) < TIM5->ARR) {}
	TIM5->SR &= ~TIM_SR_UIF;
}

void updateScore(void)
{

	sprintf(a,"%d",score);
	for (int i = 0; i < 27; i++)
	{
		if (i<7)
			displayScore[i] = display[i];
		else
			displayScore[i] = a[i-7];
	}
	lcd_clear();
	lcd_printf(displayScore);
}

void startGame(void)
{
	GPIOC->ODR = 0;
	lcd_clear();
	lcd_printf("  STARTING IN");
	delay3sec();
	lcd_clear();
	lcd_printf("       3!");
	delay3sec();
	lcd_clear();
	lcd_printf("       2!");
	delay3sec();
	lcd_clear();
	lcd_printf("       1!");
	delay3sec();
}

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM5_Init();
  MX_TIM2_Init();
  TRNG_Init();
  lcd_init_4bits(RS_GPIO_Port,RS_Pin,enable_Pin, d4_GPIO_Port,d4_Pin,d5_Pin, d6_Pin,d7_Pin);
  HAL_Delay (100);
  lcd_printf("    Welcome");
  HAL_Delay (100);
  while (1)
  {
	  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
	  {
		  gameOver = 0;
		  startGame();
		  while (gameStart)
		  {
			  updateScore();
			  gameOver = 1;
		  	  delay1sec();
		  	  GPIOD->ODR ^= (1<<12);
			  if(gameOver)
			  {
				  lcd_clear();
				  break;
			  }
			  score++;
		  }
	  }

	  if(gameOver)
	  {
		  GPIOC->ODR = 65535;
		  GPIOD->ODR |= (1<<3);

		  lcd_printf("   GAME OVER!");
		  delay3sec();
		  GPIOD->ODR &= ~(1<<3);
		  GPIOD->ODR ^= (1<<12);
		  gameStart = 0;
		  gameOver = 0;
		  score = 0;
	  }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void TRNG_Init(void) {
    // Enable RNG clock
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;

    // Enable the TRNG
    RNG->CR |= RNG_CR_RNGEN;
}

uint32_t TRNG_GetRandomNumber(uint32_t min, uint32_t max) {
    // Wait for RNG to become ready
    while (!(RNG->SR & RNG_SR_DRDY));

    // Obtain a random number
    uint32_t random_number = RNG->DR;

    // Adjust the range of the random number
    random_number = random_number % (max - min + 1) + min;

    return random_number;
}

static void MX_TIM2_Init(void)
{
	RCC->APB1ENR |= (1<<0);
	TIM2->PSC = 16799;
	TIM2->ARR = 4999;
	TIM2->CR1 |= (1<<0);
	TIM2->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
	TIM2->SR &= ~TIM_SR_UIF;  // Clear the update interrupt flag
}

static void MX_TIM5_Init(void)
{
	RCC->APB1ENR |= (1<<3);
	TIM5->PSC = 16799;
	TIM5->ARR = 4999;
	TIM5->CR1 |= (1<<0);
}

void EXTI0_IRQHandler(void)
{
	if (random_LED == 0)
	{
		gameOver = 0;
		GPIOC->ODR = 0;
	}
	else
	{
		gameOver = 1;
		GPIOC->ODR = 0;
	}
	GPIOD->ODR ^= (1<<13);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);

}

void EXTI1_IRQHandler(void)
{
	if (random_LED == 1)
	{
		gameOver = 0;
		GPIOC->ODR = 0;

	}
	else
	{
		gameOver = 1;
		GPIOC->ODR = 0;
	}
	GPIOD->ODR ^= (1<<13);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_IRQHandler(void)
{
	if (random_LED == 2)
	{
		gameOver = 0;
		GPIOC->ODR = 0;
	}
	else
	{
		gameOver = 1;
		GPIOC->ODR = 0;
	}
	GPIOD->ODR ^= (1<<13);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI3_IRQHandler(void)
{
	if (random_LED == 3)
	{
		gameOver = 0;
		GPIOC->ODR = 0;
	}
	else
	{
		gameOver = 1;
		GPIOC->ODR = 0;
	}
	GPIOD->ODR ^= (1<<13);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler(void)
{
	if (random_LED == 4)
	{
		gameOver = 0;
		GPIOC->ODR = 0;
	}
	else
	{
		gameOver = 1;
		GPIOC->ODR = 0;
	}
	GPIOD->ODR ^= (1<<13);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & (1<<5))
	{
		if (random_LED == 5)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);

		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
	}

	if (EXTI->PR & (1<<6))
	{
		if (random_LED == 6)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
	}
	if (EXTI->PR & (1<<7))
	{
		if (random_LED == 7)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
	}
	if (EXTI->PR & (1<<8))
	{
		if (random_LED == 8)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
	}
	if (EXTI->PR & (1<<9))
	{
		if (random_LED == 9)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
	}
}

void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & (1<<10))
	{
		if (random_LED == 10)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
	}

	if (EXTI->PR & (1<<11))
	{
		if (random_LED == 11)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
	}
	if (EXTI->PR & (1<<12))
	{
		if (random_LED == 12)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
	}
	if (EXTI->PR & (1<<13))
	{
		if (random_LED == 13)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
	}
	if (EXTI->PR & (1<<14))
	{
		if (random_LED == 14)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
	}
	if (EXTI->PR & (1<<15))
	{
		if (random_LED == 15)
		{
			gameOver = 0;
			GPIOC->ODR = 0;
		}
		else
		{
			gameOver = 1;
			GPIOC->ODR = 0;
		}
		GPIOD->ODR ^= (1<<13);
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
	}
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13 PC14 PC15 PC0
                           PC1 PC2 PC3 PC4
                           PC5 PC6 PC7 PC8
                           PC9 PC10 PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD, RS_Pin|enable_Pin|d4_Pin|d5_Pin
                            |d6_Pin|d7_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : RS_Pin enable_Pin d4_Pin d5_Pin
                             d6_Pin d7_Pin */
    GPIO_InitStruct.Pin = RS_Pin|enable_Pin|d4_Pin|d5_Pin
                            |d6_Pin|d7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pin : PD2 */
  	GPIO_InitStruct.Pin = GPIO_PIN_2;
  	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  	GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pin : PD3 */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB13 PB14
                           PB15 PB3 PB4 PB5
                           PB6 PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PD13 */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {

  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
