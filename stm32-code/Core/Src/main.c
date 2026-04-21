/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile uint8_t g_key1_exti_pressed_flag = 0U;
static uint32_t g_key1_last_irq_tick = 0U;
static uint8_t g_key1_override_active = 0U;
static uint8_t g_key1_release_pending = 0U;
static uint32_t g_key1_release_tick = 0U;
static led_id_t g_rb_active_led = LED_RED;
static led_id_t g_rb_saved_led = LED_RED;
static uint32_t g_rb_last_toggle_tick = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void stage3_set_red_blue_state(led_id_t active_led);
static void stage3_update_red_blue_blink(void);
static void stage3_handle_key1_press(void);
static void stage3_handle_key1_release(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void stage3_set_red_blue_state(led_id_t active_led)
{
  if (active_led == LED_RED)
  {
    led_on(LED_RED);
    led_off(LED_BLUE);
  }
  else
  {
    led_on(LED_BLUE);
    led_off(LED_RED);
  }
}

static void stage3_update_red_blue_blink(void)
{
  uint32_t now;

  if (g_key1_override_active != 0U)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_rb_last_toggle_tick) < 500U)
  {
    return;
  }

  g_rb_last_toggle_tick = now;
  if (g_rb_active_led == LED_RED)
  {
    g_rb_active_led = LED_BLUE;
  }
  else
  {
    g_rb_active_led = LED_RED;
  }

  stage3_set_red_blue_state(g_rb_active_led);
}

static void stage3_handle_key1_press(void)
{
  if (g_key1_exti_pressed_flag == 0U)
  {
    return;
  }

  g_key1_exti_pressed_flag = 0U;

  if (g_key1_override_active != 0U)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) != GPIO_PIN_RESET)
  {
    return;
  }

  g_rb_saved_led = g_rb_active_led;
  g_key1_override_active = 1U;
  g_key1_release_pending = 0U;

  led_off(LED_RED);
  led_off(LED_BLUE);
  led_on(LED_GREEN);
}

static void stage3_handle_key1_release(void)
{
  uint32_t now;

  if (g_key1_override_active == 0U)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET)
  {
    g_key1_release_pending = 0U;
    return;
  }

  now = HAL_GetTick();
  if (g_key1_release_pending == 0U)
  {
    g_key1_release_pending = 1U;
    g_key1_release_tick = now;
    return;
  }

  if ((now - g_key1_release_tick) < 30U)
  {
    return;
  }

  g_key1_release_pending = 0U;
  g_key1_override_active = 0U;
  led_off(LED_GREEN);
  g_rb_active_led = g_rb_saved_led;
  stage3_set_red_blue_state(g_rb_active_led);
  g_rb_last_toggle_tick = now;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  led_init();
  stage3_set_red_blue_state(g_rb_active_led);
  g_rb_last_toggle_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    stage3_handle_key1_press();
    stage3_handle_key1_release();
    stage3_update_red_blue_blink();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now;

  if (GPIO_Pin != KEY_1_Pin)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_key1_last_irq_tick) < 30U)
  {
    return;
  }

  g_key1_last_irq_tick = now;
  g_key1_exti_pressed_flag = 1U;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
