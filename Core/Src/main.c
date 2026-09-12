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
#include "crc.h"
#include "dcmi.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include "lcd_spi_154.h"
#include "dcmi_ov2640.h"
#include "sccb.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#if defined(__CC_ARM)
__align(32) uint16_t Camera_Buffer_Array[Display_Width * Display_Height];
#else
uint16_t Camera_Buffer_Array[Display_Width * Display_Height] __attribute__((aligned(32)));
#endif
#define Camera_Buffer ((uint32_t)Camera_Buffer_Array)
#define Camera_Buffer_SwapBytes 0U
#define Camera_Buffer_NonCacheable 0U
#define CubeAI_Template_Enable 1U
#define CubeAI_Detect_Frame_Period 1U
#define CubeAI_Max_Defer_Frames 80U
#define Display_Update_Frame_Period 3U
#define Servo_Enable 1U
#define Servo_TIM_Channel TIM_CHANNEL_4
#define Servo_Pulse_Min_Us 500U
#define Servo_Pulse_Center_Us 1500U
#define Servo_Pulse_Max_Us 2500U
#define Servo_Track_Stable_Zone_Px 16
#define Servo_Track_Update_Period_Ms 20U
#define Servo_Track_Base_Step_Us 20U
#define Servo_Track_Max_Step_Us 90U
#define Servo_Track_Invert 1U
#define Servo_Scan_Min_Pulse_Us 550U
#define Servo_Scan_Max_Pulse_Us 2450U
#define Servo_Lost_Scan_Delay_Ms 800U
#define Servo_Scan_Update_Period_Ms 60U
#define Servo_Scan_Step_Us 32U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint16_t servo_pulse_us = Servo_Pulse_Center_Us;
static uint32_t servo_last_track_tick = 0U;
static uint32_t servo_last_scan_tick = 0U;
static uint32_t servo_lost_since_tick = 0U;
static int8_t servo_scan_dir = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void LCD_ShowBootStatus(uint16_t color, const char *line1, const char *line2)
{
  LCD_SetBackColor(color);
  LCD_SetColor(LCD_COLOR_WHITE);
  LCD_FillColor(color);
  LCD_DisplayString(24U, 72U, line1);
  LCD_DisplayString(24U, 112U, line2);
}

static void LCD_ShowCameraFail(void)
{
  LCD_ShowBootStatus(LCD_COLOR_RED, "CAM ERR", "STEP:");
  LCD_DisplayNumber(120U, 112U, OV2640_InitState, 2U);
  LCD_DisplayString(24U, 152U, "ID:");
  LCD_DisplayHexNumber(72U, 152U, OV2640_LastID, 4U);
  LCD_DisplayString(24U, 192U, "ACK:");
  LCD_DisplayNumber(88U, 192U, SCCB_LastAckError, 1U);
}

static void LCD_ShowDmaFail(HAL_StatusTypeDef status)
{
  LCD_ShowBootStatus(LCD_COLOR_YELLOW, "DMA ERR", "STAT:");
  LCD_DisplayNumber(120U, 112U, (uint32_t)status, 2U);
}

static void Servo_SetPulse(uint16_t pulse_us)
{
#if (Servo_Enable != 0U)
  if (pulse_us < Servo_Pulse_Min_Us)
  {
    pulse_us = Servo_Pulse_Min_Us;
  }
  else if (pulse_us > Servo_Pulse_Max_Us)
  {
    pulse_us = Servo_Pulse_Max_Us;
  }

  servo_pulse_us = pulse_us;
  __HAL_TIM_SET_COMPARE(&htim1, Servo_TIM_Channel, servo_pulse_us);
#else
  (void)pulse_us;
#endif
}

static void Servo_Start(void)
{
#if (Servo_Enable != 0U)
  Servo_SetPulse(Servo_Pulse_Center_Us);
  (void)HAL_TIM_PWM_Start(&htim1, Servo_TIM_Channel);
#endif
}

static void Servo_AddPulseDelta(int16_t delta_us)
{
#if (Servo_Enable != 0U)
  int32_t next_pulse = (int32_t)servo_pulse_us + (int32_t)delta_us;

  if (next_pulse < (int32_t)Servo_Pulse_Min_Us)
  {
    next_pulse = (int32_t)Servo_Pulse_Min_Us;
  }
  else if (next_pulse > (int32_t)Servo_Pulse_Max_Us)
  {
    next_pulse = (int32_t)Servo_Pulse_Max_Us;
  }

  Servo_SetPulse((uint16_t)next_pulse);
#else
  (void)delta_us;
#endif
}

static void Servo_ScanWhenLost(uint32_t now)
{
#if (Servo_Enable != 0U)
  if ((now - servo_last_scan_tick) < Servo_Scan_Update_Period_Ms)
  {
    return;
  }
  servo_last_scan_tick = now;

  if (servo_scan_dir > 0)
  {
    if (servo_pulse_us >= Servo_Scan_Max_Pulse_Us)
    {
      servo_scan_dir = -1;
      Servo_AddPulseDelta(-(int16_t)Servo_Scan_Step_Us);
    }
    else
    {
      Servo_AddPulseDelta((int16_t)Servo_Scan_Step_Us);
    }
  }
  else
  {
    if (servo_pulse_us <= Servo_Scan_Min_Pulse_Us)
    {
      servo_scan_dir = 1;
      Servo_AddPulseDelta((int16_t)Servo_Scan_Step_Us);
    }
    else
    {
      Servo_AddPulseDelta(-(int16_t)Servo_Scan_Step_Us);
    }
  }
#else
  (void)now;
#endif
}

static void Servo_TrackFace(void)
{
#if (Servo_Enable != 0U)
  uint16_t face_x = 0U;
  uint16_t face_y = 0U;
  uint16_t face_w = 0U;
  uint8_t confidence = 0U;
  uint32_t now = HAL_GetTick();
  int32_t error;
  uint16_t step_us;

  if ((now - servo_last_track_tick) < Servo_Track_Update_Period_Ms)
  {
    return;
  }
  servo_last_track_tick = now;

  if (MX_X_CUBE_AI_GetPrimaryFace(&face_x, &face_y, &face_w, &confidence) == 0)
  {
    if (servo_lost_since_tick == 0U)
    {
      servo_lost_since_tick = now;
      return;
    }

    if ((now - servo_lost_since_tick) < Servo_Lost_Scan_Delay_Ms)
    {
      return;
    }

    Servo_ScanWhenLost(now);
    return;
  }

  servo_lost_since_tick = 0U;

  (void)face_y;
  (void)face_w;
  (void)confidence;

  error = (int32_t)face_x - ((int32_t)Display_Width / 2);
  if ((error >= -Servo_Track_Stable_Zone_Px) && (error <= Servo_Track_Stable_Zone_Px))
  {
    return;
  }

  if (error < 0)
  {
    step_us = (uint16_t)((-error / 24) + 1) * Servo_Track_Base_Step_Us;
    if (step_us > Servo_Track_Max_Step_Us)
    {
      step_us = Servo_Track_Max_Step_Us;
    }
#if (Servo_Track_Invert != 0U)
    Servo_AddPulseDelta((int16_t)step_us);
#else
    Servo_AddPulseDelta(-(int16_t)step_us);
#endif
  }
  else
  {
    step_us = (uint16_t)((error / 24) + 1) * Servo_Track_Base_Step_Us;
    if (step_us > Servo_Track_Max_Step_Us)
    {
      step_us = Servo_Track_Max_Step_Us;
    }
#if (Servo_Track_Invert != 0U)
    Servo_AddPulseDelta(-(int16_t)step_us);
#else
    Servo_AddPulseDelta((int16_t)step_us);
#endif
  }
#endif
}

static void User_MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x24000000UL;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.SubRegionDisable = 0x00;
#if (Camera_Buffer_NonCacheable != 0U)
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
#else
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
#endif
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
#if (Camera_Buffer_NonCacheable != 0U)
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
#else
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
#endif

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  OV2640_StatusTypeDef camera_status;
  HAL_StatusTypeDef dma_status;
  uint8_t ai_frame_divider = 0U;
  uint8_t ai_defer_count = 0U;
  uint8_t display_frame_divider = 0U;
  uint8_t app_fps = 0U;
  uint32_t app_fps_last_tick = 0U;
  uint32_t app_frame_count_1s = 0U;
#define MPU_Config User_MPU_Config

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));
  __DSB();
  __ISB();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_SPI4_Init();
  MX_USART1_UART_Init();
  MX_CRC_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  LED_Init();
  SPI_LCD_Init();
  Servo_Start();

#if (CubeAI_Template_Enable != 0U)
  LCD_ShowBootStatus(LCD_COLOR_BLUE, "AI INIT", "WAIT");
  MX_X_CUBE_AI_Init();
  if (!MX_X_CUBE_AI_IsReady())
  {
    ai_error ai_init_error = MX_X_CUBE_AI_GetLastError();
    LCD_ShowBootStatus(LCD_COLOR_RED, "AI ERR", "T:");
    LCD_DisplayHexNumber(72U, 112U, ai_init_error.type, 2U);
    LCD_DisplayString(120U, 112U, "C:");
    LCD_DisplayHexNumber(152U, 112U, ai_init_error.code, 4U);
  }
#endif

  camera_status = DCMI_OV2640_Init();

  if (camera_status == OV2640_Success)
  {
    dma_status = OV2640_DMA_Transmit_Continuous(Camera_Buffer, OV2640_BufferSize);
    if (dma_status != HAL_OK)
    {
      LCD_ShowDmaFail(dma_status);
    }
  }
  else
  {
    dma_status = HAL_ERROR;
    LCD_ShowCameraFail();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (DCMI_FrameState == 1U)
    {
	
      DCMI_FrameState = 0U;
      SCB_InvalidateDCache_by_Addr((uint32_t *)Camera_Buffer,
                                   (int32_t)(Display_Width * Display_Height * 2U));

#if (CubeAI_Template_Enable != 0U)
      if (MX_X_CUBE_AI_IsReady())
      {
        if (ai_frame_divider == 0U)
        {
          if ((DCMI_FrameState == 0U) || (ai_defer_count >= CubeAI_Max_Defer_Frames))
          {
            MX_X_CUBE_AI_Process();
            Servo_TrackFace();
            ai_frame_divider = (CubeAI_Detect_Frame_Period > 0U) ?
                                (uint8_t)(CubeAI_Detect_Frame_Period - 1U) : 0U;
            ai_defer_count = 0U;
          }
          else
          {
            ai_defer_count++;
          }
        }
        else
        {
          ai_frame_divider--;
        }
      }
#endif

      if (display_frame_divider == 0U)
      {
#if (Camera_Buffer_SwapBytes != 0U)
        LCD_CopyBufferSwapBytes(0U, 0U, Display_Width, Display_Height, (uint16_t *)Camera_Buffer);
#else
        LCD_CopyBuffer(0U, 0U, Display_Width, Display_Height, (uint16_t *)Camera_Buffer);
#endif
#if (CubeAI_Template_Enable != 0U)
        MX_X_CUBE_AI_DrawDetections();
#endif
        LCD_SetColor(LCD_COLOR_WHITE);
        LCD_DisplayStringTransparent(8U, 200U, "FPS:");
        LCD_DisplayNumberTransparent(72U, 200U, app_fps, 3U);
#if (CubeAI_Template_Enable != 0U)
        LCD_DisplayStringTransparent(128U, 200U, "AI:");
        LCD_DisplayNumberTransparent(176U, 200U, MX_X_CUBE_AI_GetLastProcessMs(), 3U);
#endif
        LED1_Toggle;
        display_frame_divider = (Display_Update_Frame_Period > 0U) ?
                                (uint8_t)(Display_Update_Frame_Period - 1U) : 0U;
      }
      else
      {
        display_frame_divider--;
      }

      app_frame_count_1s++;
      if (app_fps_last_tick == 0U)
      {
        app_fps_last_tick = HAL_GetTick();
      }
      else
      {
        uint32_t now = HAL_GetTick();
        uint32_t elapsed = now - app_fps_last_tick;
        if (elapsed >= 1000U)
        {
          uint32_t fps = (app_frame_count_1s * 1000U) / elapsed;
          app_fps = (fps > 255U) ? 255U : (uint8_t)fps;
          app_frame_count_1s = 0U;
          app_fps_last_tick = now;
        }
      }
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 8;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
#undef MPU_Config

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
