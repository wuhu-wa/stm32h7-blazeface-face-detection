#include "sccb.h"

#ifndef SCCB_STRICT_ACK
#define SCCB_STRICT_ACK 0U
#endif

#ifndef SCCB_DELAY_SCALE
#define SCCB_DELAY_SCALE 64U
#endif

#define SCCB_SCL_HIGH()  HAL_GPIO_WritePin(SCCB_SCL_GPIO_Port, SCCB_SCL_Pin, GPIO_PIN_SET)
#define SCCB_SCL_LOW()   HAL_GPIO_WritePin(SCCB_SCL_GPIO_Port, SCCB_SCL_Pin, GPIO_PIN_RESET)
#define SCCB_SDA_HIGH()  HAL_GPIO_WritePin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin, GPIO_PIN_SET)
#define SCCB_SDA_LOW()   HAL_GPIO_WritePin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin, GPIO_PIN_RESET)
#define SCCB_SDA_READ()  HAL_GPIO_ReadPin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin)

volatile uint8_t SCCB_LastAckError = 0U;
volatile uint8_t SCCB_LastRegAddr = 0U;

static void SCCB_SDA_Output(void);
static void SCCB_SDA_Input(void);

static void SCCB_Delay(void)
{
  volatile uint32_t i;

  for (i = 0U; i < (SCCB_DelayVaule * SCCB_DELAY_SCALE); i++)
  {
    __NOP();
  }
}

static void SCCB_SDA_Output(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = SCCB_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SCCB_SDA_GPIO_Port, &GPIO_InitStruct);
}

static void SCCB_SDA_Input(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = SCCB_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SCCB_SDA_GPIO_Port, &GPIO_InitStruct);
}

void SCCB_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, SCCB_SCL_Pin | SCCB_SDA_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = SCCB_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SCCB_SCL_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SCCB_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SCCB_SDA_GPIO_Port, &GPIO_InitStruct);

  SCCB_SCL_HIGH();
  SCCB_SDA_HIGH();
}

void SCCB_Start(void)
{
  SCCB_SDA_HIGH();
  SCCB_SCL_HIGH();
  SCCB_Delay();
  SCCB_SDA_LOW();
  SCCB_Delay();
  SCCB_SCL_LOW();
}

void SCCB_Stop(void)
{
  SCCB_SCL_LOW();
  SCCB_SDA_LOW();
  SCCB_Delay();
  SCCB_SCL_HIGH();
  SCCB_Delay();
  SCCB_SDA_HIGH();
  SCCB_Delay();
}

void SCCB_ACK(void)
{
  SCCB_SCL_LOW();
  SCCB_SDA_LOW();
  SCCB_Delay();
  SCCB_SCL_HIGH();
  SCCB_Delay();
  SCCB_SCL_LOW();
  SCCB_Delay();
  SCCB_SDA_HIGH();
}

void SCCB_NoACK(void)
{
  SCCB_SCL_LOW();
  SCCB_SDA_HIGH();
  SCCB_Delay();
  SCCB_SCL_HIGH();
  SCCB_Delay();
  SCCB_SCL_LOW();
  SCCB_Delay();
}

uint8_t SCCB_WaitACK(void)
{
  uint16_t timeout = 0U;
  uint8_t status = 0U;

  SCCB_SDA_HIGH();
  SCCB_SDA_Input();
  SCCB_Delay();
  SCCB_SCL_HIGH();
  SCCB_Delay();

  while (SCCB_SDA_READ() != GPIO_PIN_RESET)
  {
    timeout++;
    if (timeout > 250U)
    {
      status = 1U;
      break;
    }
    SCCB_Delay();
  }

  SCCB_SCL_LOW();
  SCCB_SDA_Output();
  SCCB_SDA_HIGH();
  SCCB_Delay();
  return status;
}

uint8_t SCCB_WriteByte(uint8_t data)
{
  uint8_t i;

  for (i = 0U; i < 8U; i++)
  {
    SCCB_SCL_LOW();
    if ((data & 0x80U) != 0U)
    {
      SCCB_SDA_HIGH();
    }
    else
    {
      SCCB_SDA_LOW();
    }
    data <<= 1U;
    SCCB_Delay();
    SCCB_SCL_HIGH();
    SCCB_Delay();
  }

  SCCB_SCL_LOW();
  return SCCB_WaitACK();
}

uint8_t SCCB_ReadByte(void)
{
  uint8_t i;
  uint8_t data = 0U;

  SCCB_SDA_HIGH();
  SCCB_SDA_Input();

  for (i = 0U; i < 8U; i++)
  {
    data <<= 1U;
    SCCB_SCL_LOW();
    SCCB_Delay();
    SCCB_SCL_HIGH();
    SCCB_Delay();
    if (SCCB_SDA_READ() != GPIO_PIN_RESET)
    {
      data |= 0x01U;
    }
  }

  SCCB_SCL_LOW();
  SCCB_SDA_Output();
  SCCB_SDA_HIGH();
  return data;
}

uint8_t SCCB_WriteReg(uint8_t addr, uint8_t value)
{
  uint8_t status = 0U;

  SCCB_LastAckError = 0U;
  SCCB_LastRegAddr = addr;

  SCCB_Start();
  if (SCCB_WriteByte(SCCB_OV2640_ADDR) != 0U)
  {
    status = 1U;
    SCCB_LastAckError = 1U;
  }
  else if (SCCB_WriteByte(addr) != 0U)
  {
    status = 1U;
    SCCB_LastAckError = 2U;
  }
  else if (SCCB_WriteByte(value) != 0U)
  {
    status = 1U;
    SCCB_LastAckError = 3U;
  }
  SCCB_Stop();

#if (SCCB_STRICT_ACK == 0U)
  status = 0U;
#endif

  return status;
}

uint8_t SCCB_ReadReg(uint8_t addr)
{
  uint8_t value;

  SCCB_Start();
  (void)SCCB_WriteByte(SCCB_OV2640_ADDR);
  (void)SCCB_WriteByte(addr);
  SCCB_Stop();

  SCCB_Start();
  (void)SCCB_WriteByte(SCCB_OV2640_ADDR | 0x01U);
  value = SCCB_ReadByte();
  SCCB_NoACK();
  SCCB_Stop();

  return value;
}
