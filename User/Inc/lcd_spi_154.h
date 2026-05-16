#ifndef __LCD_SPI_154_H__
#define __LCD_SPI_154_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#ifndef LCD_Width
#define LCD_Width       240U
#endif
#ifndef LCD_Height
#define LCD_Height      240U
#endif

#define LCD_DC_GPIO_Port  GPIOE
#define LCD_DC_Pin        GPIO_PIN_15
#define LCD_BL_GPIO_Port  GPIOD
#define LCD_BL_Pin        GPIO_PIN_15

#define LCD_COLOR_BLACK   0x0000U
#define LCD_COLOR_WHITE   0xFFFFU
#define LCD_COLOR_RED     0xF800U
#define LCD_COLOR_GREEN   0x07E0U
#define LCD_COLOR_BLUE    0x001FU
#define LCD_COLOR_YELLOW  0xFFE0U
#define LCD_COLOR_CYAN    0x07FFU

#define Direction_V       0U
#define ASCII_Font24      24U

#define LCD_Backlight_ON  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET)
#define LCD_Backlight_OFF HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET)

void SPI_LCD_Init(void);
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Clear(void);
void LCD_FillColor(uint16_t color);
void LCD_SetColor(uint16_t color);
void LCD_SetBackColor(uint16_t color);
void LCD_DisplayString(uint16_t x, uint16_t y, const char *str);
void LCD_DisplayNumber(uint16_t x, uint16_t y, uint32_t number, uint8_t len);
void LCD_DisplayHexNumber(uint16_t x, uint16_t y, uint32_t number, uint8_t len);
void LCD_DisplayStringTransparent(uint16_t x, uint16_t y, const char *str);
void LCD_DisplayNumberTransparent(uint16_t x, uint16_t y, uint32_t number, uint8_t len);
void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *buffer);
void LCD_CopyBufferSwapBytes(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *buffer);
void LCD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       uint16_t color, uint8_t thickness);
void LCD_DrawCross(uint16_t x, uint16_t y, uint16_t color);
HAL_StatusTypeDef LCD_SPI_Transmit(const uint8_t *data, uint16_t size);
HAL_StatusTypeDef LCD_SPI_TransmitBuffer(const uint16_t *data, uint32_t pixel_count);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_SPI_154_H__ */
