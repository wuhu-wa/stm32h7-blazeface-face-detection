#include "led.h"

void LED_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOC_CLK_ENABLE();

  LED1_OFF;

  GPIO_InitStruct.Pin = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);
}
