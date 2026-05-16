#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define LED1_GPIO_Port GPIOC
#define LED1_Pin       GPIO_PIN_13

#define LED1_ON        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED1_OFF       HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED1_Toggle    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin)

void LED_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H__ */
