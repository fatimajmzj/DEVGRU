/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Calibracion y arranque de ESC brushless (balancin eolico)
  *                   STM32F411 - TIM3 CH1 (PA6) y CH2 (PA7), PWM 50 Hz
  ******************************************************************************
  * Timer: tick = 1 us  ->  CCR = ancho de pulso en microsegundos
  *   PSC = 83     (reloj de TIM3 = 84 MHz; ajustar si tu reloj es otro)
  *   ARR = 19999  (periodo 20 ms = 50 Hz)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PD */
/* ===================== PARAMETROS DE PRUEBA ===================== */
#define MODO_CALIBRACION   0      /* 1 = calibrar ESC, 0 = operacion normal */

#define ESC_MIN_US         1000   /* acelerador minimo  */
#define ESC_MAX_US         2000   /* acelerador maximo  */
#define V_GIRO_US          1300   /* velocidad de prueba: subir de 100 en 100 hasta 1700 */

#define T_CALIB_MAX_MS     15000  /* tiempo con 2000 us para conectar bateria y oir tonos */
#define T_CALIB_MIN_MS     5000   /* tiempo con 1000 us para oir tonos de confirmacion    */
#define T_ARMING_MS        10000  /* tiempo con 1000 us para conectar bateria y armar     */
#define T_GIRO_MS          5000   /* tiempo de giro estable                               */
/* USER CODE END PD */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);

/* USER CODE BEGIN PFP */
static void ESC_Set(uint32_t canal, uint16_t us);
static void ESC_SetAmbos(uint16_t us);
static void ESC_Rampa(uint16_t desde, uint16_t hasta, uint16_t paso, uint32_t ms);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */

#if MODO_CALIBRACION
  /* ================= CALIBRACION =================
   * Iniciar con la BATERIA DESCONECTADA y SIN HELICES.
   * 1) El STM32 envia 2000 us
   * 2) Conectas la bateria y esperas los tonos del maximo
   * 3) Cambia a 1000 us y el ESC confirma el minimo
   */
  ESC_SetAmbos(ESC_MAX_US);                    /* maximo ANTES de iniciar el PWM */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  HAL_Delay(T_CALIB_MAX_MS);                   /* conectar bateria y esperar tonos */
  ESC_SetAmbos(ESC_MIN_US);                    /* cambiar a minimo */
  HAL_Delay(T_CALIB_MIN_MS);                   /* tonos de confirmacion */

  while (1) { }                                /* fin: desconectar bateria y poner MODO_CALIBRACION 0 */

#else
  /* ================= OPERACION NORMAL =================
   * Iniciar con la BATERIA DESCONECTADA y SIN HELICES.
   */
  ESC_SetAmbos(ESC_MIN_US);                    /* minimo ANTES de iniciar el PWM */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  HAL_Delay(T_ARMING_MS);                      /* arming: conectar bateria y esperar tonos */

  ESC_Rampa(1000, 1300, 5, 20);                /* tramo 1: 60 pasos x 20 ms = 1.2 s */
  ESC_Rampa(1300, V_GIRO_US, 10, 20);          /* tramo 2 (solo si V_GIRO_US > 1300) */
  HAL_Delay(T_GIRO_MS);                        /* giro estable */
  ESC_Rampa(V_GIRO_US, 1000, 10, 20);          /* bajada gradual */
  ESC_SetAmbos(ESC_MIN_US);                    /* paro */
#endif

  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN WHILE */
    /* Tras la prueba el ESC queda en 1000 us (motor detenido) */
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration (Nucleo-F411RE por defecto: HSI -> PLL -> 84 MHz)
  *        APB1 = 42 MHz  ->  reloj de TIM3 = 84 MHz
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization: PWM 50 Hz, tick de 1 us
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;                         /* (84 MHz / 1 MHz) - 1 */
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19999;                         /* 20 ms = 50 Hz */
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1000;                            /* arranca en minimo (1000 us) */
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim3);                       /* configura PA6 y PA7 como AF2 (TIM3) */
}

/**
  * @brief GPIO Initialization (solo habilita relojes de puertos)
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */

/* Fija el ancho de pulso (us) en un canal, limitado a 1000-2000 us */
static void ESC_Set(uint32_t canal, uint16_t us)
{
  if (us < ESC_MIN_US) us = ESC_MIN_US;
  if (us > ESC_MAX_US) us = ESC_MAX_US;
  __HAL_TIM_SET_COMPARE(&htim3, canal, us);          /* CCR = us */
}

/* Mismo pulso para los dos motores */
static void ESC_SetAmbos(uint16_t us)
{
  ESC_Set(TIM_CHANNEL_1, us);
  ESC_Set(TIM_CHANNEL_2, us);
}

/* Rampa lineal de 'desde' a 'hasta' en pasos de 'paso' us, cada 'ms' milisegundos */
static void ESC_Rampa(uint16_t desde, uint16_t hasta, uint16_t paso, uint32_t ms)
{
  if (desde <= hasta)
  {
    for (uint16_t p = desde; p < hasta; p += paso)
    {
      ESC_SetAmbos(p);
      HAL_Delay(ms);
    }
  }
  else
  {
    for (uint16_t p = desde; p > hasta; p -= paso)
    {
      ESC_SetAmbos(p);
      HAL_Delay(ms);
    }
  }
  ESC_SetAmbos(hasta);
}

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
