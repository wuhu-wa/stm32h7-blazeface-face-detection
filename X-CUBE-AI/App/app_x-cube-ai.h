
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_AI_H
#define __APP_AI_H
#ifdef __cplusplus
extern "C" {
#endif
/**
  ******************************************************************************
  * @file    app_x-cube-ai.h
  * @author  X-CUBE-AI C code generator
  * @brief   AI entry function definitions
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
/* Includes ------------------------------------------------------------------*/
#include "ai_platform.h"

void MX_X_CUBE_AI_Init(void);
void MX_X_CUBE_AI_Process(void);
/* USER CODE BEGIN includes */
int MX_X_CUBE_AI_IsReady(void);
ai_error MX_X_CUBE_AI_GetLastError(void);
const char *MX_X_CUBE_AI_GetLastErrorFunction(void);
uint32_t MX_X_CUBE_AI_GetLastProcessMs(void);
void MX_X_CUBE_AI_DrawDetections(void);
int MX_X_CUBE_AI_GetPrimaryFace(uint16_t *center_x, uint16_t *center_y,
                                uint16_t *box_width, uint8_t *confidence);
/* USER CODE END includes */
#ifdef __cplusplus
}
#endif
#endif /*__STMicroelectronics_X-CUBE-AI_9_1_0_H */
