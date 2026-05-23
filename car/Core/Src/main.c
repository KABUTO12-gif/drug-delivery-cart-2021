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
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "log.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_ARR_MAX        (3599U)
#define MOTOR_TEST_RIGHT   (720U)
#define MOTOR_L_COMP_NUM   (81U)
#define MOTOR_L_COMP_DEN   (100U)
#define LINE_CTRL_PERIOD_MS (20U)
#define LINE_BASE_PWM       (680)
#define LINE_KP             (55)
#define PWM_ARR_MAX_I32     (3599)
#define IR_DRUG_ACTIVE_LEVEL (0U)
#define BEEP_ACTIVE_LEVEL    (GPIO_PIN_RESET) /* low-level trigger */
#define BEEP_IDLE_LEVEL      (GPIO_PIN_SET)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t s_last_batt_log_ms = 0;
static uint32_t s_last_motor_log_ms = 0;
static uint32_t s_last_gray_log_ms = 0;
static uint32_t s_last_line_ctrl_ms = 0;
static uint32_t s_last_ir_log_ms = 0;
static uint32_t s_last_beep_tick_ms = 0;
static int32_t s_prev_enc_l = 0;
static int32_t s_prev_enc_r = 0;
static uint32_t s_last_oled_ms = 0;
static int32_t s_last_line_err = 0;
static uint8_t s_last_gray_mask = 0;
static uint8_t s_drug_present = 0;
static uint16_t s_cmd_pwm_l = 0;
static uint16_t s_cmd_pwm_r = 0;
static uint8_t s_beep_on = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Motor_RuntimeInit(void);
static void Motor_SetDuty(uint16_t left, uint16_t right);
static void Motor_Stop(void);
static void Motor_SetForward(void);
static void Motor_ApplyTrackPWM(int32_t base_pwm, int32_t turn_pwm);
static void Motor_ResetEncBase(void);
static void OLED_ShowBoot(void);
static void OLED_ShowRuntime(uint16_t bat_mv, uint8_t gray_mask, int32_t err, uint8_t drug_present);
static uint8_t Gray_ReadMask(void);
static int32_t Line_CalcError(uint8_t gray_mask);
static void LineFollow_Update(void);
static uint8_t IR_DrugPresent(void);
static void LED_Update(uint8_t drug_present);
static void BEEP_Set(uint8_t on);
static void BEEP_Update(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_ShowBoot();
  LOGI("System boot, clock=%lu Hz", HAL_RCC_GetHCLKFreq());
  Motor_RuntimeInit();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if ((HAL_GetTick() - s_last_batt_log_ms) >= 1000U)
    {
      uint16_t raw = ADC1_ReadRaw();
      uint32_t vadc_mv = ((uint32_t)raw * 3300U) / 4095U;
      s_last_batt_log_ms = HAL_GetTick();
      LOGI("BAT raw=%u vadc=%lumV", raw, vadc_mv);
      OLED_ShowRuntime((uint16_t)vadc_mv, s_last_gray_mask, s_last_line_err, s_drug_present);
    }
    if ((HAL_GetTick() - s_last_motor_log_ms) >= 2000U)
    {
      s_last_motor_log_ms = HAL_GetTick();
      LOGI("ENC L=%ld R=%ld", (int32_t)__HAL_TIM_GET_COUNTER(&htim2), (int32_t)__HAL_TIM_GET_COUNTER(&htim4));
    }
    if ((HAL_GetTick() - s_last_line_ctrl_ms) >= LINE_CTRL_PERIOD_MS)
    {
      s_last_line_ctrl_ms = HAL_GetTick();
      s_drug_present = IR_DrugPresent();
      LED_Update(s_drug_present);
      if (s_drug_present != 0U)
      {
        LineFollow_Update();
      }
      else
      {
        Motor_Stop();
      }
    }
    BEEP_Update();
    if ((HAL_GetTick() - s_last_gray_log_ms) >= 200U)
    {
      uint8_t g = s_last_gray_mask;
      s_last_gray_log_ms = HAL_GetTick();
      LOGI("GRAY mask=0x%02X b=%u%u%u%u%u%u%u%u",
           g,
           (g >> 7) & 0x1U, (g >> 6) & 0x1U, (g >> 5) & 0x1U, (g >> 4) & 0x1U,
           (g >> 3) & 0x1U, (g >> 2) & 0x1U, (g >> 1) & 0x1U, g & 0x1U);
      LOGI("LINE err=%ld", s_last_line_err);
    }
    if ((HAL_GetTick() - s_last_ir_log_ms) >= 500U)
    {
      s_last_ir_log_ms = HAL_GetTick();
      LOGI("IR drug=%u", (unsigned int)s_drug_present);
    }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void Motor_SetDuty(uint16_t left, uint16_t right)
{
  s_cmd_pwm_l = left;
  s_cmd_pwm_r = right;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, left);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, right);
}

static void Motor_Stop(void)
{
  Motor_SetDuty(0, 0);
}

static void Motor_SetForward(void)
{
  HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
}

static void Motor_ApplyTrackPWM(int32_t base_pwm, int32_t turn_pwm)
{
  int32_t left_raw = base_pwm - turn_pwm;
  int32_t right_raw = base_pwm + turn_pwm;
  int32_t left_comp;

  if (left_raw < 0) left_raw = 0;
  if (left_raw > PWM_ARR_MAX_I32) left_raw = PWM_ARR_MAX_I32;
  if (right_raw < 0) right_raw = 0;
  if (right_raw > PWM_ARR_MAX_I32) right_raw = PWM_ARR_MAX_I32;

  left_comp = (left_raw * MOTOR_L_COMP_NUM) / MOTOR_L_COMP_DEN;
  if (left_comp > PWM_ARR_MAX_I32) left_comp = PWM_ARR_MAX_I32;

  Motor_SetDuty((uint16_t)left_comp, (uint16_t)right_raw);
}

static void Motor_RuntimeInit(void)
{
  HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET);

  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    LOGE("PWM CH1 start failed");
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
  {
    LOGE("PWM CH2 start failed");
  }
  if (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK)
  {
    LOGE("ENC TIM2 start failed");
  }
  if (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK)
  {
    LOGE("ENC TIM4 start failed");
  }

  __HAL_TIM_SET_COUNTER(&htim2, 0);
  __HAL_TIM_SET_COUNTER(&htim4, 0);
  Motor_ResetEncBase();
  s_drug_present = IR_DrugPresent();
  LED_Update(s_drug_present);
  BEEP_Set(0);
  Motor_SetForward();
  Motor_Stop();
  LOGI("Motor runtime init done, compensation L=%u/%u", (unsigned int)MOTOR_L_COMP_NUM, (unsigned int)MOTOR_L_COMP_DEN);
}

static void Motor_ResetEncBase(void)
{
  s_prev_enc_l = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
  s_prev_enc_r = (int32_t)__HAL_TIM_GET_COUNTER(&htim4);
}

static void OLED_ShowBoot(void)
{
  OLED_Clear();
  OLED_PrintAt(0, 0, "Drug Cart 2021");
  OLED_PrintAt(2, 0, "STM32F103VET6");
  OLED_PrintAt(4, 0, "OLED Ready");
  OLED_Update();
}

static void OLED_ShowRuntime(uint16_t bat_mv, uint8_t gray_mask, int32_t err, uint8_t drug_present)
{
  char line[22];
  uint32_t now = HAL_GetTick();

  if ((now - s_last_oled_ms) < 500U)
  {
    return;
  }
  s_last_oled_ms = now;

  OLED_Clear();

  snprintf(line, sizeof(line), "BAT:%4umV", (unsigned int)bat_mv);
  OLED_PrintAt(0, 0, line);

  snprintf(line, sizeof(line), "ENC L:%6ld", (long)((int32_t)__HAL_TIM_GET_COUNTER(&htim2)));
  OLED_PrintAt(2, 0, line);

  snprintf(line, sizeof(line), "ENC R:%6ld", (long)((int32_t)__HAL_TIM_GET_COUNTER(&htim4)));
  OLED_PrintAt(4, 0, line);

  snprintf(line, sizeof(line), "Lcmp:%u/%u", (unsigned int)MOTOR_L_COMP_NUM, (unsigned int)MOTOR_L_COMP_DEN);
  OLED_PrintAt(5, 0, line);

  snprintf(line, sizeof(line), "G:%02X E:%ld", gray_mask, (long)err);
  OLED_PrintAt(6, 0, line);
  OLED_PrintAt(7, 0, (drug_present != 0U) ? "Drug:IN  LED:G" : "Drug:OUT LED:R");

  OLED_Update();
}

static uint8_t Gray_ReadMask(void)
{
  uint8_t m = 0;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_1_GPIO_Port, GRAY_1_Pin) & 0x1U) << 0;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_2_GPIO_Port, GRAY_2_Pin) & 0x1U) << 1;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_3_GPIO_Port, GRAY_3_Pin) & 0x1U) << 2;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_4_GPIO_Port, GRAY_4_Pin) & 0x1U) << 3;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_5_GPIO_Port, GRAY_5_Pin) & 0x1U) << 4;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_6_GPIO_Port, GRAY_6_Pin) & 0x1U) << 5;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_7_GPIO_Port, GRAY_7_Pin) & 0x1U) << 6;
  m |= ((uint8_t)HAL_GPIO_ReadPin(GRAY_8_GPIO_Port, GRAY_8_Pin) & 0x1U) << 7;
  return m;
}

static int32_t Line_CalcError(uint8_t gray_mask)
{
  static const int8_t weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};
  int32_t sum = 0;
  int32_t cnt = 0;
  uint8_t i;
  for (i = 0; i < 8; i++)
  {
    if (((gray_mask >> i) & 0x1U) != 0U)
    {
      sum += weights[i];
      cnt++;
    }
  }
  if (cnt == 0)
  {
    return s_last_line_err;
  }
  return sum / cnt;
}

static void LineFollow_Update(void)
{
  uint8_t g = Gray_ReadMask();
  int32_t err = Line_CalcError(g);
  int32_t turn = LINE_KP * err;

  s_last_gray_mask = g;
  s_last_line_err = err;

  Motor_ApplyTrackPWM(LINE_BASE_PWM, turn);
}

static uint8_t IR_DrugPresent(void)
{
  uint8_t ir = (uint8_t)HAL_GPIO_ReadPin(IR_DRUG_DET_GPIO_Port, IR_DRUG_DET_Pin);
  return (uint8_t)(ir == IR_DRUG_ACTIVE_LEVEL);
}

static void LED_Update(uint8_t drug_present)
{
  HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, (drug_present != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, (drug_present != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void BEEP_Set(uint8_t on)
{
  s_beep_on = (on != 0U) ? 1U : 0U;
  HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, s_beep_on ? BEEP_ACTIVE_LEVEL : BEEP_IDLE_LEVEL);
}

static void BEEP_Update(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t should_alert = 0;

  /* pickup reminder: drug detected and chassis is stopped */
  if ((s_drug_present != 0U) && (s_cmd_pwm_l == 0U) && (s_cmd_pwm_r == 0U))
  {
    should_alert = 1U;
  }

  if (should_alert == 0U)
  {
    BEEP_Set(0);
    s_last_beep_tick_ms = now;
    return;
  }

  /* 150ms ON + 350ms OFF */
  if (s_beep_on != 0U)
  {
    if ((now - s_last_beep_tick_ms) >= 150U)
    {
      BEEP_Set(0);
      s_last_beep_tick_ms = now;
    }
  }
  else
  {
    if ((now - s_last_beep_tick_ms) >= 350U)
    {
      BEEP_Set(1);
      s_last_beep_tick_ms = now;
    }
  }
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
