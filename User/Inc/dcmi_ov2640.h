#ifndef __DCMI_OV2640_H__
#define __DCMI_OV2640_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#ifndef LCD_Width
#define LCD_Width           240U
#endif
#ifndef LCD_Height
#define LCD_Height          240U
#endif

#define OV2640_Width        400U
#define OV2640_Height       300U
#define Display_Width       LCD_Width
#define Display_Height      LCD_Height
#define OV2640_BufferSize   ((uint32_t)(Display_Width * Display_Height * 2U / 4U))

#define OV2640_REG_END      0xFFU
#define OV2640_VAL_END      0xFFU

typedef enum
{
  OV2640_Failed = 0U,
  OV2640_Success = 1U
} OV2640_StatusTypeDef;

typedef struct
{
  uint8_t reg;
  uint8_t val;
} OV2640_RegTypeDef;

extern volatile uint8_t DCMI_FrameState;
extern volatile uint8_t OV2640_FPS;
extern volatile uint32_t DCMI_Frame_Count;
extern volatile uint32_t DCMI_Error_Count;
extern volatile uint32_t DCMI_Last_ErrorCode;
extern volatile uint32_t DCMI_Vsync_Count;
extern volatile uint32_t DCMI_Line_Count;
extern volatile uint16_t OV2640_LastID;
extern volatile uint8_t OV2640_InitState;

#define OV2640_INIT_STATE_OK          0U
#define OV2640_INIT_STATE_RESET_FAIL  1U
#define OV2640_INIT_STATE_ID_FAIL     2U
#define OV2640_INIT_STATE_SVGA_FAIL   3U
#define OV2640_INIT_STATE_RGB_FAIL    4U
#define OV2640_INIT_STATE_SIZE_FAIL   5U
#define OV2640_INIT_STATE_CROP_FAIL   6U

extern const OV2640_RegTypeDef OV2640_RGB565_Config[];
extern const OV2640_RegTypeDef OV2640_SVGA_Config[];

OV2640_StatusTypeDef OV2640_Reset(void);
uint16_t OV2640_ReadID(void);
OV2640_StatusTypeDef OV2640_Config(const OV2640_RegTypeDef *config);
OV2640_StatusTypeDef OV2640_Set_Framesize(uint16_t width, uint16_t height);
OV2640_StatusTypeDef OV2640_DCMI_Crop(uint16_t display_width, uint16_t display_height,
                                      uint16_t sensor_width, uint16_t sensor_height);
void OV2640_DCMI_SetSyncPolarity(uint32_t hs_polarity, uint32_t vs_polarity, uint32_t pck_polarity);
OV2640_StatusTypeDef DCMI_OV2640_Init(void);
HAL_StatusTypeDef OV2640_DMA_Transmit_Continuous(uint32_t buffer_addr, uint32_t buffer_size_words);
void OV2640_DCMI_EnableDiagnostics(void);
uint32_t OV2640_DCMI_GetSR(void);
uint32_t OV2640_DCMI_GetRIS(void);
uint32_t OV2640_DCMI_GetCR(void);
uint32_t OV2640_DCMI_GetIER(void);
uint32_t OV2640_DMA_GetNDTR(void);

#ifdef __cplusplus
}
#endif

#endif /* __DCMI_OV2640_H__ */
