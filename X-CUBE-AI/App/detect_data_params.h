/**
  ******************************************************************************
  * @file    detect_data_params.h
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-05-16T17:56:07+0800
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#ifndef DETECT_DATA_PARAMS_H
#define DETECT_DATA_PARAMS_H

#include "ai_platform.h"

/*
#define AI_DETECT_DATA_WEIGHTS_PARAMS \
  (AI_HANDLE_PTR(&ai_detect_data_weights_params[1]))
*/

#define AI_DETECT_DATA_CONFIG               (NULL)


#define AI_DETECT_DATA_ACTIVATIONS_SIZES \
  { 327680, }
#define AI_DETECT_DATA_ACTIVATIONS_SIZE     (327680)
#define AI_DETECT_DATA_ACTIVATIONS_COUNT    (1)
#define AI_DETECT_DATA_ACTIVATION_1_SIZE    (327680)



#define AI_DETECT_DATA_WEIGHTS_SIZES \
  { 107956, }
#define AI_DETECT_DATA_WEIGHTS_SIZE         (107956)
#define AI_DETECT_DATA_WEIGHTS_COUNT        (1)
#define AI_DETECT_DATA_WEIGHT_1_SIZE        (107956)



#define AI_DETECT_DATA_ACTIVATIONS_TABLE_GET() \
  (&g_detect_activations_table[1])

extern ai_handle g_detect_activations_table[1 + 2];



#define AI_DETECT_DATA_WEIGHTS_TABLE_GET() \
  (&g_detect_weights_table[1])

extern ai_handle g_detect_weights_table[1 + 2];


#endif    /* DETECT_DATA_PARAMS_H */
