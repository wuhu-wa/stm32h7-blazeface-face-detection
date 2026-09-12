
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
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

 /*
  * Description
  *   v1.0 - Minimum template to show how to use the Embedded Client API
  *          model. Only one input and one output is supported. All
  *          memory resources are allocated statically (AI_NETWORK_XX, defines
  *          are used).
  *          Re-target of the printf function is out-of-scope.
  *   v2.0 - add multiple IO and/or multiple heap support
  *
  *   For more information, see the embeded documentation:
  *
  *       [1] %X_CUBE_AI_DIR%/Documentation/index.html
  *
  *   X_CUBE_AI_DIR indicates the location where the X-CUBE-AI pack is installed
  *   typical : C:\Users\<user_name>\STM32Cube\Repository\STMicroelectronics\X-CUBE-AI\7.1.0
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#endif

/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "detect.h"
#include "detect_data.h"

/* USER CODE BEGIN includes */
#include <math.h>
#include "dcmi_ov2640.h"
#include "lcd_spi_154.h"

extern uint16_t Camera_Buffer_Array[];

#define FACE_DETECT_INPUT_W       AI_DETECT_IN_1_WIDTH
#define FACE_DETECT_INPUT_H       AI_DETECT_IN_1_HEIGHT
#define FACE_DETECT_SCORE_TH      0.70f
#define FACE_DETECT_SCORE_LOGIT_TH 0.84729786f
#define FACE_DETECT_NMS_TH        0.30f
#define FACE_DETECT_MAX_CAND      16U
#define FACE_DETECT_MAX_RESULTS   4U
#define FACE_DETECT_KEYPOINTS     6U
#define FACE_DETECT_X_SCALE       128.0f
#define FACE_DETECT_Y_SCALE       128.0f
#define FACE_DETECT_W_SCALE       128.0f
#define FACE_DETECT_H_SCALE       128.0f

typedef struct
{
  float score;
  int16_t x1;
  int16_t y1;
  int16_t x2;
  int16_t y2;
  int16_t key_x[FACE_DETECT_KEYPOINTS];
  int16_t key_y[FACE_DETECT_KEYPOINTS];
} face_detection_t;

static face_detection_t face_candidates[FACE_DETECT_MAX_CAND];
static face_detection_t face_results[FACE_DETECT_MAX_RESULTS];
static uint16_t face_input_src_x[FACE_DETECT_INPUT_W];
static uint32_t face_input_src_row[FACE_DETECT_INPUT_H];
static float face_rgb5_to_float[32];
static float face_rgb6_to_float[64];
static uint8_t face_candidate_count = 0U;
static uint8_t face_result_count = 0U;
static uint8_t face_preprocess_map_ready = 0U;
static uint32_t ai_last_process_ms = 0U;

#if defined(__CC_ARM)
__align(32) static uint8_t pool0_raw[AI_DETECT_DATA_ACTIVATION_1_SIZE + 31U];
#else
static uint8_t pool0_raw[AI_DETECT_DATA_ACTIVATION_1_SIZE + 31U] __attribute__((aligned(32)));
#endif

#define AI_ALIGN_32_PTR(ptr_) \
  ((ai_handle)((((uint32_t)(ptr_)) + 31U) & ~31U))

static int ai_ready = 0;
static volatile ai_error ai_last_error = {AI_ERROR_NONE, AI_ERROR_CODE_NONE};
static const char *ai_last_error_fct = NULL;

static float face_sigmoid(float value);
static float face_iou(const face_detection_t *a, const face_detection_t *b);
static void face_prepare_preprocess_map(void);
static void face_get_anchor(uint16_t index, float *cx, float *cy);
static int16_t face_to_screen(float value, uint16_t max_value);
static void face_insert_candidate(const face_detection_t *det);
static void face_decode_one(const float *box, float raw_score, uint16_t anchor_index);
static void face_run_nms(void);
static void face_draw_label(uint16_t x, uint16_t y, uint8_t confidence);

/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/

#if !defined(AI_DETECT_INPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_in_1[AI_DETECT_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_DETECT_IN_NUM] = {
data_in_1
};
#else
ai_i8* data_ins[AI_DETECT_IN_NUM] = {
NULL
};
#endif

#if !defined(AI_DETECT_OUTPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_out_1[AI_DETECT_OUT_1_SIZE_BYTES];
AI_ALIGNED(4) ai_i8 data_out_2[AI_DETECT_OUT_2_SIZE_BYTES];
AI_ALIGNED(4) ai_i8 data_out_3[AI_DETECT_OUT_3_SIZE_BYTES];
AI_ALIGNED(4) ai_i8 data_out_4[AI_DETECT_OUT_4_SIZE_BYTES];
ai_i8* data_outs[AI_DETECT_OUT_NUM] = {
data_out_1,
data_out_2,
data_out_3,
data_out_4
};
#else
ai_i8* data_outs[AI_DETECT_OUT_NUM] = {
NULL,
NULL,
NULL,
NULL
};
#endif

/* Activations buffers -------------------------------------------------------*/

ai_handle data_activations0[] = {AI_HANDLE_NULL};

/* AI objects ----------------------------------------------------------------*/

static ai_handle detect = AI_HANDLE_NULL;

static ai_buffer* ai_input;
static ai_buffer* ai_output;

static void ai_log_err(const ai_error err, const char *fct)
{
  /* USER CODE BEGIN log */
  ai_last_error = err;
  ai_last_error_fct = fct;
  ai_ready = 0;

  if (fct)
    printf("TEMPLATE - Error (%s) - type=0x%02x code=0x%02x\r\n", fct,
        err.type, err.code);
  else
    printf("TEMPLATE - Error - type=0x%02x code=0x%02x\r\n", err.type, err.code);
  /* USER CODE END log */
}

static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err;

  ai_ready = 0;

  if ((act_addr == NULL) || (act_addr[0] == AI_HANDLE_NULL)) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_NETWORK_ACTIVATIONS;
    ai_log_err(err, "activations_null");
    return -1;
  }

  if ((((uint32_t)act_addr[0]) & 0x1FU) != 0U) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_NETWORK_ACTIVATIONS;
    ai_log_err(err, "activations_align");
    return -1;
  }

  /* Create and initialize an instance of the model */
  err = ai_detect_create_and_init(&detect, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_detect_create_and_init");
    return -1;
  }

  ai_input = ai_detect_inputs_get(detect, NULL);
  ai_output = ai_detect_outputs_get(detect, NULL);

  if ((ai_input == NULL) || (ai_output == NULL)) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_INVALID_PTR;
    ai_log_err(err, "io_desc_null");
    return -1;
  }

#if defined(AI_DETECT_INPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-inputs" option is used, memory buffer can be
   *  used from the activations buffer. This is not mandatory.
   */
  for (int idx=0; idx < AI_DETECT_IN_NUM; idx++) {
	data_ins[idx] = ai_input[idx].data;
  }
#else
  for (int idx=0; idx < AI_DETECT_IN_NUM; idx++) {
	  ai_input[idx].data = data_ins[idx];
  }
#endif

#if defined(AI_DETECT_OUTPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-outputs" option is used, memory buffer can be
   *  used from the activations buffer. This is no mandatory.
   */
  for (int idx=0; idx < AI_DETECT_OUT_NUM; idx++) {
	data_outs[idx] = ai_output[idx].data;
  }
#else
  for (int idx=0; idx < AI_DETECT_OUT_NUM; idx++) {
	ai_output[idx].data = data_outs[idx];
  }
#endif

  if ((data_ins[0] == NULL) || ((((uint32_t)data_ins[0]) & 0x03U) != 0U)) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_INVALID_PTR;
    ai_log_err(err, "input_ptr");
    return -1;
  }

  for (int idx=0; idx < AI_DETECT_OUT_NUM; idx++) {
    if ((data_outs[idx] == NULL) || ((((uint32_t)data_outs[idx]) & 0x03U) != 0U)) {
      err.type = AI_ERROR_INIT_FAILED;
      err.code = AI_ERROR_CODE_INVALID_PTR;
      ai_log_err(err, "output_ptr");
      return -1;
    }
  }

  ai_ready = 1;
  return 0;
}

static int ai_run(void)
{
  ai_i32 batch;

  batch = ai_detect_run(detect, ai_input, ai_output);
  if (batch != 1) {
    ai_log_err(ai_detect_get_error(detect),
        "ai_detect_run");
    return -1;
  }

  return 0;
}

/* USER CODE BEGIN 2 */
static int ai_bootstrap_safe(ai_handle *act_addr)
{
  ai_error err;

  ai_ready = 0;

  if ((act_addr == NULL) || (act_addr[0] == AI_HANDLE_NULL)) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_NETWORK_ACTIVATIONS;
    ai_log_err(err, "activations_null");
    return -1;
  }

  if ((((uint32_t)act_addr[0]) & 0x1FU) != 0U) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_NETWORK_ACTIVATIONS;
    ai_log_err(err, "activations_align");
    return -1;
  }

  err = ai_detect_create_and_init(&detect, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_detect_create_and_init");
    return -1;
  }

  ai_input = ai_detect_inputs_get(detect, NULL);
  ai_output = ai_detect_outputs_get(detect, NULL);

  if ((ai_input == NULL) || (ai_output == NULL)) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_INVALID_PTR;
    ai_log_err(err, "io_desc_null");
    return -1;
  }

#if defined(AI_DETECT_INPUTS_IN_ACTIVATIONS)
  for (int idx=0; idx < AI_DETECT_IN_NUM; idx++) {
    data_ins[idx] = ai_input[idx].data;
  }
#else
  for (int idx=0; idx < AI_DETECT_IN_NUM; idx++) {
    ai_input[idx].data = data_ins[idx];
  }
#endif

#if defined(AI_DETECT_OUTPUTS_IN_ACTIVATIONS)
  for (int idx=0; idx < AI_DETECT_OUT_NUM; idx++) {
    data_outs[idx] = ai_output[idx].data;
  }
#else
  for (int idx=0; idx < AI_DETECT_OUT_NUM; idx++) {
    ai_output[idx].data = data_outs[idx];
  }
#endif

  if ((data_ins[0] == NULL) || ((((uint32_t)data_ins[0]) & 0x03U) != 0U)) {
    err.type = AI_ERROR_INIT_FAILED;
    err.code = AI_ERROR_CODE_INVALID_PTR;
    ai_log_err(err, "input_ptr");
    return -1;
  }

  for (int idx=0; idx < AI_DETECT_OUT_NUM; idx++) {
    if ((data_outs[idx] == NULL) || ((((uint32_t)data_outs[idx]) & 0x03U) != 0U)) {
      err.type = AI_ERROR_INIT_FAILED;
      err.code = AI_ERROR_CODE_INVALID_PTR;
      ai_log_err(err, "output_ptr");
      return -1;
    }
  }

  ai_ready = 1;
  return 0;
}

static float face_sigmoid(float value)
{
  if (value < -80.0f)
  {
    return 0.0f;
  }
  if (value > 80.0f)
  {
    return 1.0f;
  }

  return 1.0f / (1.0f + expf(-value));
}

static float face_iou(const face_detection_t *a, const face_detection_t *b)
{
  int16_t ix1;
  int16_t iy1;
  int16_t ix2;
  int16_t iy2;
  int32_t inter;
  int32_t area_a;
  int32_t area_b;
  int32_t denom;

  ix1 = (a->x1 > b->x1) ? a->x1 : b->x1;
  iy1 = (a->y1 > b->y1) ? a->y1 : b->y1;
  ix2 = (a->x2 < b->x2) ? a->x2 : b->x2;
  iy2 = (a->y2 < b->y2) ? a->y2 : b->y2;

  if ((ix2 <= ix1) || (iy2 <= iy1))
  {
    return 0.0f;
  }

  inter = (int32_t)(ix2 - ix1) * (int32_t)(iy2 - iy1);
  area_a = (int32_t)(a->x2 - a->x1) * (int32_t)(a->y2 - a->y1);
  area_b = (int32_t)(b->x2 - b->x1) * (int32_t)(b->y2 - b->y1);
  denom = area_a + area_b - inter;

  if (denom <= 0)
  {
    return 0.0f;
  }

  return (float)inter / (float)denom;
}

static void face_prepare_preprocess_map(void)
{
  uint16_t i;

  for (i = 0U; i < FACE_DETECT_INPUT_W; i++)
  {
    face_input_src_x[i] = (uint16_t)(((uint32_t)i * Display_Width) / FACE_DETECT_INPUT_W);
  }

  for (i = 0U; i < FACE_DETECT_INPUT_H; i++)
  {
    uint16_t src_y = (uint16_t)(((uint32_t)i * Display_Height) / FACE_DETECT_INPUT_H);
    face_input_src_row[i] = (uint32_t)src_y * Display_Width;
  }

  for (i = 0U; i < 32U; i++)
  {
    face_rgb5_to_float[i] = (float)((i << 3U) | (i >> 2U)) * (1.0f / 255.0f);
  }

  for (i = 0U; i < 64U; i++)
  {
    face_rgb6_to_float[i] = (float)((i << 2U) | (i >> 4U)) * (1.0f / 255.0f);
  }

  face_preprocess_map_ready = 1U;
}

static void face_get_anchor(uint16_t index, float *cx, float *cy)
{
  uint16_t local;
  uint16_t cell;
  uint16_t grid;
  uint16_t anchors_per_cell;

  if (index < 512U)
  {
    local = index;
    grid = 16U;
    anchors_per_cell = 2U;
  }
  else
  {
    local = (uint16_t)(index - 512U);
    grid = 8U;
    anchors_per_cell = 6U;
  }

  cell = (uint16_t)(local / anchors_per_cell);
  *cx = ((float)(cell % grid) + 0.5f) / (float)grid;
  *cy = ((float)(cell / grid) + 0.5f) / (float)grid;
}

static int16_t face_to_screen(float value, uint16_t max_value)
{
  int32_t pixel;

  pixel = (int32_t)(value * (float)max_value + 0.5f);
  if (pixel < 0)
  {
    pixel = 0;
  }
  if (pixel >= (int32_t)max_value)
  {
    pixel = (int32_t)max_value - 1;
  }

  return (int16_t)pixel;
}

static void face_insert_candidate(const face_detection_t *det)
{
  uint8_t pos = face_candidate_count;
  uint8_t i;

  if (pos >= FACE_DETECT_MAX_CAND)
  {
    if (det->score <= face_candidates[FACE_DETECT_MAX_CAND - 1U].score)
    {
      return;
    }
    pos = FACE_DETECT_MAX_CAND - 1U;
  }
  else
  {
    face_candidate_count++;
  }

  while ((pos > 0U) && (det->score > face_candidates[pos - 1U].score))
  {
    face_candidates[pos] = face_candidates[pos - 1U];
    pos--;
  }

  face_candidates[pos] = *det;

  for (i = (uint8_t)(pos + 1U); i < face_candidate_count; i++)
  {
    if (face_candidates[i].score > face_candidates[i - 1U].score)
    {
      face_detection_t tmp = face_candidates[i - 1U];
      face_candidates[i - 1U] = face_candidates[i];
      face_candidates[i] = tmp;
    }
  }
}

static void face_decode_one(const float *box, float raw_score, uint16_t anchor_index)
{
  face_detection_t det;
  float score;
  float anchor_x;
  float anchor_y;
  float x_center;
  float y_center;
  float width;
  float height;
  uint8_t k;

  if (raw_score < FACE_DETECT_SCORE_LOGIT_TH)
  {
    return;
  }

  score = face_sigmoid(raw_score);
  if (score < FACE_DETECT_SCORE_TH)
  {
    return;
  }

  face_get_anchor(anchor_index, &anchor_x, &anchor_y);

  x_center = (box[0] / FACE_DETECT_X_SCALE) + anchor_x;
  y_center = (box[1] / FACE_DETECT_Y_SCALE) + anchor_y;
  width = box[2] / FACE_DETECT_W_SCALE;
  height = box[3] / FACE_DETECT_H_SCALE;

  det.score = score;
  det.x1 = face_to_screen(x_center - (width * 0.5f), Display_Width);
  det.y1 = face_to_screen(y_center - (height * 0.5f), Display_Height);
  det.x2 = face_to_screen(x_center + (width * 0.5f), Display_Width);
  det.y2 = face_to_screen(y_center + (height * 0.5f), Display_Height);

  if (((det.x2 - det.x1) < 4) || ((det.y2 - det.y1) < 4))
  {
    return;
  }

  for (k = 0U; k < FACE_DETECT_KEYPOINTS; k++)
  {
    float key_x = (box[4U + (2U * k)] / FACE_DETECT_X_SCALE) + anchor_x;
    float key_y = (box[5U + (2U * k)] / FACE_DETECT_Y_SCALE) + anchor_y;
    det.key_x[k] = face_to_screen(key_x, Display_Width);
    det.key_y[k] = face_to_screen(key_y, Display_Height);
  }

  face_insert_candidate(&det);
}

static void face_run_nms(void)
{
  uint8_t i;
  uint8_t j;

  face_result_count = 0U;

  for (i = 0U; (i < face_candidate_count) && (face_result_count < FACE_DETECT_MAX_RESULTS); i++)
  {
    uint8_t keep = 1U;

    for (j = 0U; j < face_result_count; j++)
    {
      if (face_iou(&face_candidates[i], &face_results[j]) > FACE_DETECT_NMS_TH)
      {
        keep = 0U;
        break;
      }
    }

    if (keep != 0U)
    {
      face_results[face_result_count] = face_candidates[i];
      face_result_count++;
    }
  }
}

static void face_draw_label(uint16_t x, uint16_t y, uint8_t confidence)
{
  uint16_t label_y = (y >= 24U) ? (uint16_t)(y - 24U) : y;

  LCD_SetColor(LCD_COLOR_YELLOW);
  LCD_DisplayStringTransparent(x, label_y, "C:");
  LCD_DisplayNumberTransparent((uint16_t)(x + 32U), label_y, confidence, 3U);
}

int acquire_and_process_data(ai_i8* data[])
{
  float *input;
  uint16_t y;
  uint16_t x;

  if ((data == NULL) || (data[0] == NULL)) {
    return -1;
  }

  if ((((uint32_t)data[0]) & 0x03U) != 0U) {
    return -1;
  }

  if (face_preprocess_map_ready == 0U)
  {
    face_prepare_preprocess_map();
  }

  input = (float *)data[0];

  for (y = 0U; y < FACE_DETECT_INPUT_H; y++)
  {
    uint32_t row_offset = face_input_src_row[y];

    for (x = 0U; x < FACE_DETECT_INPUT_W; x++)
    {
      uint16_t pixel = Camera_Buffer_Array[row_offset + face_input_src_x[x]];
      *input++ = face_rgb5_to_float[(pixel >> 11U) & 0x1FU];
      *input++ = face_rgb6_to_float[(pixel >> 5U) & 0x3FU];
      *input++ = face_rgb5_to_float[pixel & 0x1FU];
    }
  }

  return 0;
}

int post_process(ai_i8* data[])
{
  const float *boxes_16;
  const float *scores_16;
  const float *scores_8;
  const float *boxes_8;
  uint16_t i;

  if ((data == NULL) || (data[0] == NULL) || (data[1] == NULL) ||
      (data[2] == NULL) || (data[3] == NULL))
  {
    return -1;
  }

  boxes_16 = (const float *)data[0];
  scores_16 = (const float *)data[1];
  scores_8 = (const float *)data[2];
  boxes_8 = (const float *)data[3];

  face_candidate_count = 0U;
  face_result_count = 0U;

  for (i = 0U; i < 512U; i++)
  {
    face_decode_one(&boxes_16[(uint32_t)i * 16U], scores_16[i], i);
  }

  for (i = 0U; i < 384U; i++)
  {
    face_decode_one(&boxes_8[(uint32_t)i * 16U], scores_8[i], (uint16_t)(i + 512U));
  }

  face_run_nms();

  return 0;
}

int MX_X_CUBE_AI_GetPrimaryFace(uint16_t *center_x, uint16_t *center_y,
                                uint16_t *box_width, uint8_t *confidence)
{
  const face_detection_t *det;

  if (face_result_count == 0U)
  {
    return 0;
  }

  det = &face_results[0];

  if (center_x != NULL)
  {
    *center_x = (uint16_t)(((int32_t)det->x1 + (int32_t)det->x2) / 2);
  }

  if (center_y != NULL)
  {
    *center_y = (uint16_t)(((int32_t)det->y1 + (int32_t)det->y2) / 2);
  }

  if (box_width != NULL)
  {
    *box_width = (uint16_t)(det->x2 - det->x1);
  }

  if (confidence != NULL)
  {
    *confidence = (uint8_t)(det->score * 100.0f + 0.5f);
  }

  return 1;
}

void MX_X_CUBE_AI_DrawDetections(void)
{
  uint8_t i;

  for (i = 0U; i < face_result_count; i++)
  {
    const face_detection_t *det = &face_results[i];
    uint16_t x = (uint16_t)det->x1;
    uint16_t y = (uint16_t)det->y1;
    uint16_t width = (uint16_t)(det->x2 - det->x1);
    uint16_t height = (uint16_t)(det->y2 - det->y1);
    uint8_t confidence = (uint8_t)(det->score * 100.0f + 0.5f);
    uint8_t k;

    LCD_DrawRectangle(x, y, width, height, LCD_COLOR_GREEN, 2U);
    face_draw_label(x, y, confidence);

    for (k = 0U; k < FACE_DETECT_KEYPOINTS; k++)
    {
      LCD_DrawCross((uint16_t)det->key_x[k], (uint16_t)det->key_y[k], LCD_COLOR_CYAN);
    }
  }
}

int MX_X_CUBE_AI_IsReady(void)
{
  return (ai_ready != 0);
}

ai_error MX_X_CUBE_AI_GetLastError(void)
{
  return ai_last_error;
}

const char *MX_X_CUBE_AI_GetLastErrorFunction(void)
{
  return ai_last_error_fct;
}

uint32_t MX_X_CUBE_AI_GetLastProcessMs(void)
{
  return (ai_last_process_ms > 999U) ? 999U : ai_last_process_ms;
}
/* USER CODE END 2 */

/* Entry points --------------------------------------------------------------*/

void MX_X_CUBE_AI_Init(void)
{
    /* USER CODE BEGIN 5 */
  ai_error err = {AI_ERROR_NONE, AI_ERROR_CODE_NONE};

  ai_last_error = err;
  ai_last_error_fct = NULL;

  data_activations0[0] = AI_ALIGN_32_PTR(pool0_raw);
  face_prepare_preprocess_map();

  printf("\r\nTEMPLATE - initialization\r\n");

  if (ai_bootstrap_safe(data_activations0) != 0) {
    if (ai_last_error.type == AI_ERROR_NONE) {
      err.type = AI_ERROR_INIT_FAILED;
      err.code = AI_ERROR_CODE_NETWORK;
      ai_log_err(err, "ai_bootstrap_safe");
    }
  }
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */
  int res = -1;
  uint32_t start_tick;

  if (!ai_ready || (detect == AI_HANDLE_NULL)) {
    return;
  }

  start_tick = HAL_GetTick();

  res = acquire_and_process_data(data_ins);
  if (res == 0)
    res = ai_run();
  if (res == 0)
    res = post_process(data_outs);

  ai_last_process_ms = HAL_GetTick() - start_tick;

  if (res) {
    ai_error err = {AI_ERROR_INVALID_STATE, AI_ERROR_CODE_NETWORK};
    ai_log_err(err, "Process has FAILED");
  }
    /* USER CODE END 6 */
}

#ifdef __cplusplus
}
#endif
