#include "dcmi_ov2640.h"
#include "dcmi.h"
#include "sccb.h"

#define OV2640_PWDN_GPIO_Port GPIOD
#define OV2640_PWDN_Pin       GPIO_PIN_14

#define OV2640_PWDN_ON()      HAL_GPIO_WritePin(OV2640_PWDN_GPIO_Port, OV2640_PWDN_Pin, GPIO_PIN_SET)
#define OV2640_PWDN_OFF()     HAL_GPIO_WritePin(OV2640_PWDN_GPIO_Port, OV2640_PWDN_Pin, GPIO_PIN_RESET)

#ifndef OV2640_ENABLE_PA8_MCO_XCLK
#define OV2640_ENABLE_PA8_MCO_XCLK 1U
#endif

#ifndef OV2640_DCMI_HSPOLARITY
#define OV2640_DCMI_HSPOLARITY DCMI_HSPOLARITY_LOW
#endif

#ifndef OV2640_DCMI_VSPOLARITY
#define OV2640_DCMI_VSPOLARITY DCMI_VSPOLARITY_LOW
#endif

#ifndef OV2640_DCMI_PCKPOLARITY
#define OV2640_DCMI_PCKPOLARITY DCMI_PCKPOLARITY_RISING
#endif

volatile uint8_t DCMI_FrameState = 0U;
volatile uint8_t OV2640_FPS = 0U;
volatile uint32_t DCMI_Frame_Count = 0U;
volatile uint32_t DCMI_Error_Count = 0U;
volatile uint32_t DCMI_Last_ErrorCode = 0U;
volatile uint32_t DCMI_Vsync_Count = 0U;
volatile uint32_t DCMI_Line_Count = 0U;
volatile uint16_t OV2640_LastID = 0U;
volatile uint8_t OV2640_InitState = OV2640_INIT_STATE_OK;

extern DMA_HandleTypeDef hdma_dcmi;

const OV2640_RegTypeDef OV2640_RGB565_Config[] =
{
  {0xFFU, 0x00U}, {0xDAU, 0x09U}, {0xD7U, 0x03U}, {0xDFU, 0x02U},
  {0x33U, 0xA0U}, {0x3CU, 0x00U}, {0xE1U, 0x67U},
  {0xFFU, 0x01U}, {0xE0U, 0x00U}, {0xE1U, 0x00U}, {0xE5U, 0x00U},
  {0xD7U, 0x00U}, {0xDAU, 0x00U}, {0xE0U, 0x00U},
  {OV2640_REG_END, OV2640_VAL_END}
};

const OV2640_RegTypeDef OV2640_SVGA_Config[] =
{
  {0xFFU, 0x00U}, {0x2CU, 0xFFU}, {0x2EU, 0xDFU},
  {0xFFU, 0x01U}, {0x3CU, 0x32U}, {0x11U, 0x00U}, {0x09U, 0x02U},
  {0x04U, 0x28U}, {0x13U, 0xE5U}, {0x14U, 0x48U}, {0x2CU, 0x0CU},
  {0x33U, 0x78U}, {0x3AU, 0x33U}, {0x3BU, 0xFBU}, {0x3EU, 0x00U},
  {0x43U, 0x11U}, {0x16U, 0x10U}, {0x39U, 0x92U}, {0x35U, 0xDAU},
  {0x22U, 0x1AU}, {0x37U, 0xC3U}, {0x23U, 0x00U}, {0x34U, 0xC0U},
  {0x36U, 0x1AU}, {0x06U, 0x88U}, {0x07U, 0xC0U}, {0x0DU, 0x87U},
  {0x0EU, 0x41U}, {0x4CU, 0x00U}, {0x48U, 0x00U}, {0x5BU, 0x00U},
  {0x42U, 0x03U}, {0x4AU, 0x81U}, {0x21U, 0x99U}, {0x24U, 0x40U},
  {0x25U, 0x38U}, {0x26U, 0x82U}, {0x5CU, 0x00U}, {0x63U, 0x00U},
  {0x46U, 0x3FU}, {0x0CU, 0x3CU}, {0x61U, 0x70U}, {0x62U, 0x80U},
  {0x7CU, 0x05U}, {0x20U, 0x80U}, {0x28U, 0x30U}, {0x6CU, 0x00U},
  {0x6DU, 0x80U}, {0x6EU, 0x00U}, {0x70U, 0x02U}, {0x71U, 0x94U},
  {0x73U, 0xC1U}, {0x3DU, 0x34U}, {0x5AU, 0x57U}, {0x12U, 0x40U},
  {0x17U, 0x11U}, {0x18U, 0x43U}, {0x19U, 0x00U}, {0x1AU, 0x4BU},
  {0x32U, 0x09U}, {0x37U, 0xC0U}, {0x4FU, 0xCAU}, {0x50U, 0xA8U},
  {0x5AU, 0x23U}, {0x6DU, 0x00U}, {0x3DU, 0x38U},

  {0xFFU, 0x00U}, {0xE5U, 0x7FU}, {0xF9U, 0xC0U}, {0x41U, 0x24U},
  {0xE0U, 0x14U}, {0x76U, 0xFFU}, {0x33U, 0xA0U}, {0x42U, 0x20U},
  {0x43U, 0x18U}, {0x4CU, 0x00U}, {0x87U, 0xD5U}, {0x88U, 0x3FU},
  {0xD7U, 0x03U}, {0xD9U, 0x10U}, {0xD3U, 0x82U}, {0xC8U, 0x08U},
  {0xC9U, 0x80U}, {0x7CU, 0x00U}, {0x7DU, 0x00U}, {0x7CU, 0x03U},
  {0x7DU, 0x48U}, {0x7DU, 0x48U}, {0x7CU, 0x08U}, {0x7DU, 0x20U},
  {0x7DU, 0x10U}, {0x7DU, 0x0EU}, {0x90U, 0x00U}, {0x91U, 0x0EU},
  {0x91U, 0x1AU}, {0x91U, 0x31U}, {0x91U, 0x5AU}, {0x91U, 0x69U},
  {0x91U, 0x75U}, {0x91U, 0x7EU}, {0x91U, 0x88U}, {0x91U, 0x8FU},
  {0x91U, 0x96U}, {0x91U, 0xA3U}, {0x91U, 0xAFU}, {0x91U, 0xC4U},
  {0x91U, 0xD7U}, {0x91U, 0xE8U}, {0x91U, 0x20U}, {0x92U, 0x00U},
  {0x93U, 0x06U}, {0x93U, 0xE3U}, {0x93U, 0x05U}, {0x93U, 0x05U},
  {0x93U, 0x00U}, {0x93U, 0x04U}, {0x93U, 0x00U}, {0x93U, 0x00U},
  {0x93U, 0x00U}, {0x93U, 0x00U}, {0x93U, 0x00U}, {0x93U, 0x00U},
  {0x93U, 0x00U}, {0x96U, 0x00U}, {0x97U, 0x08U}, {0x97U, 0x19U},
  {0x97U, 0x02U}, {0x97U, 0x0CU}, {0x97U, 0x24U}, {0x97U, 0x30U},
  {0x97U, 0x28U}, {0x97U, 0x26U}, {0x97U, 0x02U}, {0x97U, 0x98U},
  {0x97U, 0x80U}, {0x97U, 0x00U}, {0x97U, 0x00U}, {0xC3U, 0xEDU},
  {0xA4U, 0x00U}, {0xA8U, 0x00U}, {0xC5U, 0x11U}, {0xC6U, 0x51U},
  {0xBFU, 0x80U}, {0xC7U, 0x10U}, {0xB6U, 0x66U}, {0xB8U, 0xA5U},
  {0xB7U, 0x64U}, {0xB9U, 0x7CU}, {0xB3U, 0xAFU}, {0xB4U, 0x97U},
  {0xB5U, 0xFFU}, {0xB0U, 0xC5U}, {0xB1U, 0x94U}, {0xB2U, 0x0FU},
  {0xC4U, 0x5CU}, {0xC0U, 0x64U}, {0xC1U, 0x4BU}, {0x8CU, 0x00U},
  {0x86U, 0x3DU}, {0x50U, 0x89U}, {0x51U, 0xC8U}, {0x52U, 0x96U},
  {0x53U, 0x00U}, {0x54U, 0x00U}, {0x55U, 0x00U}, {0x5AU, 0xC8U},
  {0x5BU, 0x96U}, {0x5CU, 0x00U}, {0xD3U, 0x02U}, {0xC3U, 0xEDU},
  {0x7FU, 0x00U}, {0xDAU, 0x09U}, {0xE5U, 0x1FU}, {0xE1U, 0x67U},
  {0xE0U, 0x00U}, {0xDDU, 0x7FU}, {0x05U, 0x00U},
  {OV2640_REG_END, OV2640_VAL_END}
};

static void OV2640_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOD_CLK_ENABLE();

  OV2640_PWDN_ON();

  GPIO_InitStruct.Pin = OV2640_PWDN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OV2640_PWDN_GPIO_Port, &GPIO_InitStruct);
}

static void OV2640_XCLK_Config(void)
{
#if (OV2640_ENABLE_PA8_MCO_XCLK != 0U)
  __HAL_RCC_GPIOA_CLK_ENABLE();
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
  HAL_Delay(5U);
#endif
}

OV2640_StatusTypeDef OV2640_Reset(void)
{
  HAL_Delay(5U);
  OV2640_PWDN_OFF();
  HAL_Delay(5U);

  if (SCCB_WriteReg(0xFFU, 0x01U) != 0U)
  {
    return OV2640_Failed;
  }
  if (SCCB_WriteReg(0x12U, 0x80U) != 0U)
  {
    return OV2640_Failed;
  }
  HAL_Delay(10U);

  return OV2640_Success;
}

uint16_t OV2640_ReadID(void)
{
  uint8_t midh;
  uint8_t midl;

  (void)SCCB_WriteReg(0xFFU, 0x01U);
  midh = SCCB_ReadReg(0x0AU);
  midl = SCCB_ReadReg(0x0BU);

  OV2640_LastID = (uint16_t)(((uint16_t)midh << 8U) | midl);
  return OV2640_LastID;
}

OV2640_StatusTypeDef OV2640_Config(const OV2640_RegTypeDef *config)
{
  const OV2640_RegTypeDef *reg = config;

  if (config == 0)
  {
    return OV2640_Failed;
  }

  while (!((reg->reg == OV2640_REG_END) && (reg->val == OV2640_VAL_END)))
  {
    if (SCCB_WriteReg(reg->reg, reg->val) != 0U)
    {
      return OV2640_Failed;
    }
    reg++;
  }

  return OV2640_Success;
}

OV2640_StatusTypeDef OV2640_Set_Framesize(uint16_t width, uint16_t height)
{
  uint8_t reg5c;

  if ((width == 0U) || (height == 0U))
  {
    return OV2640_Failed;
  }

  reg5c = (uint8_t)(((width & 0x03U) << 6U) | ((height & 0x03U) << 4U));

  if (SCCB_WriteReg(0xFFU, 0x00U) != 0U)
  {
    return OV2640_Failed;
  }
  if (SCCB_WriteReg(0xE0U, 0x04U) != 0U)
  {
    return OV2640_Failed;
  }
  if (SCCB_WriteReg(0x5AU, (uint8_t)(width >> 2U)) != 0U)
  {
    return OV2640_Failed;
  }
  if (SCCB_WriteReg(0x5BU, (uint8_t)(height >> 2U)) != 0U)
  {
    return OV2640_Failed;
  }
  if (SCCB_WriteReg(0x5CU, reg5c) != 0U)
  {
    return OV2640_Failed;
  }
  if (SCCB_WriteReg(0xE0U, 0x00U) != 0U)
  {
    return OV2640_Failed;
  }

  return OV2640_Success;
}

OV2640_StatusTypeDef OV2640_DCMI_Crop(uint16_t display_width, uint16_t display_height,
                                      uint16_t sensor_width, uint16_t sensor_height)
{
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t cap_count;
  uint32_t vline;

  if ((display_width == 0U) || (display_height == 0U) ||
      (display_width > sensor_width) || (display_height > sensor_height))
  {
    return OV2640_Failed;
  }

  x_offset = ((uint32_t)(sensor_width - display_width) / 2U) * 2U;
  y_offset = ((uint32_t)(sensor_height - display_height) / 2U);
  if (y_offset > 0U)
  {
    y_offset--;
  }
  cap_count = ((uint32_t)display_width * 2U) - 1U;
  vline = (uint32_t)display_height - 1U;

  if (HAL_DCMI_ConfigCrop(&hdcmi, x_offset, y_offset, cap_count, vline) != HAL_OK)
  {
    return OV2640_Failed;
  }
  if (HAL_DCMI_EnableCrop(&hdcmi) != HAL_OK)
  {
    return OV2640_Failed;
  }

  return OV2640_Success;
}

void OV2640_DCMI_SetSyncPolarity(uint32_t hs_polarity, uint32_t vs_polarity, uint32_t pck_polarity)
{
  hdcmi.Init.HSPolarity = hs_polarity;
  hdcmi.Init.VSPolarity = vs_polarity;
  hdcmi.Init.PCKPolarity = pck_polarity;
  MODIFY_REG(hdcmi.Instance->CR, DCMI_CR_HSPOL | DCMI_CR_VSPOL | DCMI_CR_PCKPOL,
             hs_polarity | vs_polarity | pck_polarity);
}

OV2640_StatusTypeDef DCMI_OV2640_Init(void)
{
  uint16_t id;

  OV2640_LastID = 0U;
  OV2640_InitState = OV2640_INIT_STATE_OK;
  OV2640_GPIO_Config();
  OV2640_XCLK_Config();
  SCCB_GPIO_Config();

  if (OV2640_Reset() != OV2640_Success)
  {
    OV2640_InitState = OV2640_INIT_STATE_RESET_FAIL;
    return OV2640_Failed;
  }

  id = OV2640_ReadID();
  if ((id != 0x2640U) && (id != 0x2642U))
  {
    OV2640_InitState = OV2640_INIT_STATE_ID_FAIL;
    return OV2640_Failed;
  }

  if (OV2640_Config(OV2640_SVGA_Config) != OV2640_Success)
  {
    OV2640_InitState = OV2640_INIT_STATE_SVGA_FAIL;
    return OV2640_Failed;
  }
  if (OV2640_Config(OV2640_RGB565_Config) != OV2640_Success)
  {
    OV2640_InitState = OV2640_INIT_STATE_RGB_FAIL;
    return OV2640_Failed;
  }
  if (OV2640_Set_Framesize(OV2640_Width, OV2640_Height) != OV2640_Success)
  {
    OV2640_InitState = OV2640_INIT_STATE_SIZE_FAIL;
    return OV2640_Failed;
  }
  if (OV2640_DCMI_Crop(Display_Width, Display_Height, OV2640_Width, OV2640_Height) != OV2640_Success)
  {
    OV2640_InitState = OV2640_INIT_STATE_CROP_FAIL;
    return OV2640_Failed;
  }
  OV2640_DCMI_SetSyncPolarity(OV2640_DCMI_HSPOLARITY,
                              OV2640_DCMI_VSPOLARITY,
                              OV2640_DCMI_PCKPOLARITY);

  return OV2640_Success;
}

HAL_StatusTypeDef OV2640_DMA_Transmit_Continuous(uint32_t buffer_addr, uint32_t buffer_size_words)
{
  __HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC | DCMI_IT_FRAME);
  __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_FRAMERI | DCMI_FLAG_OVRRI |
                               DCMI_FLAG_ERRRI | DCMI_FLAG_VSYNCRI |
                               DCMI_FLAG_LINERI);
  __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_ERR | DCMI_IT_OVR);

  SCB_InvalidateDCache_by_Addr((uint32_t *)buffer_addr, (int32_t)(buffer_size_words * 4U));
  return HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, buffer_addr, buffer_size_words);
}

void OV2640_DCMI_EnableDiagnostics(void)
{
  __HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC |
                               DCMI_IT_FRAME | DCMI_IT_ERR |
                               DCMI_IT_OVR);
  __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_FRAMERI | DCMI_FLAG_OVRRI |
                               DCMI_FLAG_ERRRI | DCMI_FLAG_VSYNCRI |
                               DCMI_FLAG_LINERI);
  __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_ERR | DCMI_IT_OVR);
}

uint32_t OV2640_DCMI_GetSR(void)
{
  return hdcmi.Instance->SR;
}

uint32_t OV2640_DCMI_GetRIS(void)
{
  return hdcmi.Instance->RISR;
}

uint32_t OV2640_DCMI_GetCR(void)
{
  return hdcmi.Instance->CR;
}

uint32_t OV2640_DCMI_GetIER(void)
{
  return hdcmi.Instance->IER;
}

uint32_t OV2640_DMA_GetNDTR(void)
{
  return ((DMA_Stream_TypeDef *)hdma_dcmi.Instance)->NDTR;
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi_arg)
{
  static uint32_t last_tick = 0U;
  static uint32_t frame_count_1s = 0U;
  uint32_t now;
  uint32_t elapsed;

  if ((hdcmi_arg == 0) || (hdcmi_arg->Instance != DCMI))
  {
    return;
  }

  DCMI_Frame_Count++;
  frame_count_1s++;
  DCMI_FrameState = 1U;

  now = HAL_GetTick();
  if (last_tick == 0U)
  {
    last_tick = now;
  }

  elapsed = now - last_tick;
  if (elapsed >= 1000U)
  {
    uint32_t fps = (frame_count_1s * 1000U) / elapsed;
    OV2640_FPS = (fps > 255U) ? 255U : (uint8_t)fps;
    frame_count_1s = 0U;
    last_tick = now;
  }

}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi_arg)
{
  if ((hdcmi_arg == 0) || (hdcmi_arg->Instance != DCMI))
  {
    return;
  }

  DCMI_Error_Count++;
  DCMI_Last_ErrorCode = HAL_DCMI_GetError(hdcmi_arg);
}

void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi_arg)
{
  if ((hdcmi_arg == 0) || (hdcmi_arg->Instance != DCMI))
  {
    return;
  }

  DCMI_Vsync_Count++;
}

void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi_arg)
{
  if ((hdcmi_arg == 0) || (hdcmi_arg->Instance != DCMI))
  {
    return;
  }

  DCMI_Line_Count++;
}
