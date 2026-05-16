#include "lcd_spi_154.h"
#include "spi.h"

#define LCD_SPI             hspi4
#define LCD_SPI_TIMEOUT     1000U
#define LCD_CHAR_WIDTH      16U
#define LCD_CHAR_HEIGHT     24U
#define LCD_FONT_SCALE      3U

#define LCD_DC_COMMAND()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()       HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

static uint16_t lcd_pen_color = LCD_COLOR_WHITE;
static uint16_t lcd_back_color = LCD_COLOR_BLACK;

#if defined(__CC_ARM)
__align(4) static uint16_t lcd_line_buffer[LCD_Width];
__align(4) static uint16_t lcd_char_buffer[LCD_CHAR_WIDTH * LCD_CHAR_HEIGHT];
#else
static uint16_t lcd_line_buffer[LCD_Width] __attribute__((aligned(4)));
static uint16_t lcd_char_buffer[LCD_CHAR_WIDTH * LCD_CHAR_HEIGHT] __attribute__((aligned(4)));
#endif

static void LCD_GPIO_Config(void);
static void LCD_WriteCommand(uint8_t command);
static void LCD_WriteData(const uint8_t *data, uint16_t size);
static void LCD_WriteData8(uint8_t data);
static void LCD_SetDataSize(uint32_t data_size);
static void LCD_DrawChar(uint16_t x, uint16_t y, char ch);
static void LCD_DrawCharTransparent(uint16_t x, uint16_t y, char ch);
static void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t width, uint16_t color);
static void LCD_DrawVLine(uint16_t x, uint16_t y, uint16_t height, uint16_t color);
static const uint8_t *LCD_GetGlyph(char ch);

static const uint8_t glyph_blank[5] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
static const uint8_t glyph_colon[5] = {0x00U, 0x36U, 0x36U, 0x00U, 0x00U};
static const uint8_t glyph_letters[26][5] =
{
  {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU}, /* A */
  {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U}, /* B */
  {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U}, /* C */
  {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU}, /* D */
  {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U}, /* E */
  {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U}, /* F */
  {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU}, /* G */
  {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU}, /* H */
  {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U}, /* I */
  {0x20U, 0x40U, 0x41U, 0x3FU, 0x01U}, /* J */
  {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U}, /* K */
  {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U}, /* L */
  {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU}, /* M */
  {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU}, /* N */
  {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU}, /* O */
  {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U}, /* P */
  {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU}, /* Q */
  {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U}, /* R */
  {0x46U, 0x49U, 0x49U, 0x49U, 0x31U}, /* S */
  {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U}, /* T */
  {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU}, /* U */
  {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU}, /* V */
  {0x3FU, 0x40U, 0x38U, 0x40U, 0x3FU}, /* W */
  {0x63U, 0x14U, 0x08U, 0x14U, 0x63U}, /* X */
  {0x07U, 0x08U, 0x70U, 0x08U, 0x07U}, /* Y */
  {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}  /* Z */
};
static const uint8_t glyph_digits[10][5] =
{
  {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
  {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
  {0x42U, 0x61U, 0x51U, 0x49U, 0x46U},
  {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U},
  {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
  {0x27U, 0x45U, 0x45U, 0x45U, 0x39U},
  {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U},
  {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
  {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
  {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU}
};

static void LCD_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BL_GPIO_Port, &GPIO_InitStruct);
}

static void LCD_SetDataSize(uint32_t data_size)
{
  if (LCD_SPI.Init.DataSize == data_size)
  {
    return;
  }

  __HAL_SPI_DISABLE(&LCD_SPI);
  MODIFY_REG(LCD_SPI.Instance->CFG1, SPI_CFG1_DSIZE | SPI_CFG1_FTHLV,
             data_size | SPI_FIFO_THRESHOLD_01DATA);
  LCD_SPI.Init.DataSize = data_size;
  LCD_SPI.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
}

HAL_StatusTypeDef LCD_SPI_Transmit(const uint8_t *data, uint16_t size)
{
  if ((data == 0) || (size == 0U))
  {
    return HAL_OK;
  }

  LCD_SetDataSize(SPI_DATASIZE_8BIT);
  return HAL_SPI_Transmit(&LCD_SPI, data, size, LCD_SPI_TIMEOUT);
}

HAL_StatusTypeDef LCD_SPI_TransmitBuffer(const uint16_t *data, uint32_t pixel_count)
{
  HAL_StatusTypeDef status = HAL_OK;
  const uint16_t *ptr = data;

  if ((data == 0) || (pixel_count == 0U))
  {
    return HAL_OK;
  }

  LCD_SetDataSize(SPI_DATASIZE_16BIT);

  while ((pixel_count > 0U) && (status == HAL_OK))
  {
    uint16_t chunk = (pixel_count > 0xFFFFU) ? 0xFFFFU : (uint16_t)pixel_count;
    status = HAL_SPI_Transmit(&LCD_SPI, (const uint8_t *)ptr, chunk, HAL_MAX_DELAY);
    ptr += chunk;
    pixel_count -= chunk;
  }

  LCD_SetDataSize(SPI_DATASIZE_8BIT);
  return status;
}

static void LCD_WriteCommand(uint8_t command)
{
  LCD_DC_COMMAND();
  (void)LCD_SPI_Transmit(&command, 1U);
}

static void LCD_WriteData(const uint8_t *data, uint16_t size)
{
  LCD_DC_DATA();
  (void)LCD_SPI_Transmit(data, size);
}

static void LCD_WriteData8(uint8_t data)
{
  LCD_WriteData(&data, 1U);
}

void SPI_LCD_Init(void)
{
  static const uint8_t b2_data[5] = {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U};
  static const uint8_t d0_data[2] = {0xA4U, 0xA1U};
  static const uint8_t e0_data[14] =
  {
    0xD0U, 0x04U, 0x0DU, 0x11U, 0x13U, 0x2BU, 0x3FU,
    0x54U, 0x4CU, 0x18U, 0x0DU, 0x0BU, 0x1FU, 0x23U
  };
  static const uint8_t e1_data[14] =
  {
    0xD0U, 0x04U, 0x0CU, 0x11U, 0x13U, 0x2CU, 0x3FU,
    0x44U, 0x51U, 0x2FU, 0x1FU, 0x1FU, 0x20U, 0x23U
  };

  LCD_GPIO_Config();
  LCD_Backlight_OFF;
  HAL_Delay(20U);

  LCD_WriteCommand(0x36U);
  LCD_WriteData8(0x00U);
  LCD_WriteCommand(0x3AU);
  LCD_WriteData8(0x05U);
  LCD_WriteCommand(0xB2U);
  LCD_WriteData(b2_data, sizeof(b2_data));
  LCD_WriteCommand(0xB7U);
  LCD_WriteData8(0x35U);
  LCD_WriteCommand(0xBBU);
  LCD_WriteData8(0x19U);
  LCD_WriteCommand(0xC0U);
  LCD_WriteData8(0x2CU);
  LCD_WriteCommand(0xC2U);
  LCD_WriteData8(0x01U);
  LCD_WriteCommand(0xC3U);
  LCD_WriteData8(0x12U);
  LCD_WriteCommand(0xC4U);
  LCD_WriteData8(0x20U);
  LCD_WriteCommand(0xC6U);
  LCD_WriteData8(0x0FU);
  LCD_WriteCommand(0xD0U);
  LCD_WriteData(d0_data, sizeof(d0_data));
  LCD_WriteCommand(0xE0U);
  LCD_WriteData(e0_data, sizeof(e0_data));
  LCD_WriteCommand(0xE1U);
  LCD_WriteData(e1_data, sizeof(e1_data));
  LCD_WriteCommand(0x21U);
  LCD_WriteCommand(0x11U);
  HAL_Delay(120U);
  LCD_WriteCommand(0x29U);

  (void)Direction_V;
  (void)ASCII_Font24;
  LCD_SetColor(LCD_COLOR_WHITE);
  LCD_SetBackColor(LCD_COLOR_BLACK);
  LCD_Clear();
  LCD_Backlight_ON;
}

void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  uint8_t data[4];

  if (x1 >= LCD_Width)
  {
    x1 = LCD_Width - 1U;
  }
  if (x2 >= LCD_Width)
  {
    x2 = LCD_Width - 1U;
  }
  if (y1 >= LCD_Height)
  {
    y1 = LCD_Height - 1U;
  }
  if (y2 >= LCD_Height)
  {
    y2 = LCD_Height - 1U;
  }

  LCD_WriteCommand(0x2AU);
  data[0] = (uint8_t)(x1 >> 8U);
  data[1] = (uint8_t)x1;
  data[2] = (uint8_t)(x2 >> 8U);
  data[3] = (uint8_t)x2;
  LCD_WriteData(data, 4U);

  LCD_WriteCommand(0x2BU);
  data[0] = (uint8_t)(y1 >> 8U);
  data[1] = (uint8_t)y1;
  data[2] = (uint8_t)(y2 >> 8U);
  data[3] = (uint8_t)y2;
  LCD_WriteData(data, 4U);

  LCD_WriteCommand(0x2CU);
}

void LCD_Clear(void)
{
  uint16_t i;
  uint16_t y;

  for (i = 0U; i < LCD_Width; i++)
  {
    lcd_line_buffer[i] = lcd_back_color;
  }

  LCD_SetAddress(0U, 0U, LCD_Width - 1U, LCD_Height - 1U);
  LCD_DC_DATA();
  for (y = 0U; y < LCD_Height; y++)
  {
    (void)LCD_SPI_TransmitBuffer(lcd_line_buffer, LCD_Width);
  }
}

void LCD_FillColor(uint16_t color)
{
  uint16_t i;
  uint16_t y;

  for (i = 0U; i < LCD_Width; i++)
  {
    lcd_line_buffer[i] = color;
  }

  LCD_SetAddress(0U, 0U, LCD_Width - 1U, LCD_Height - 1U);
  LCD_DC_DATA();
  for (y = 0U; y < LCD_Height; y++)
  {
    (void)LCD_SPI_TransmitBuffer(lcd_line_buffer, LCD_Width);
  }
}

void LCD_SetColor(uint16_t color)
{
  lcd_pen_color = color;
}

void LCD_SetBackColor(uint16_t color)
{
  lcd_back_color = color;
}

void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *buffer)
{
  if ((buffer == 0) || (width == 0U) || (height == 0U))
  {
    return;
  }

  if ((x >= LCD_Width) || (y >= LCD_Height))
  {
    return;
  }

  if ((x + width) > LCD_Width)
  {
    width = LCD_Width - x;
  }
  if ((y + height) > LCD_Height)
  {
    height = LCD_Height - y;
  }

  LCD_SetAddress(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  LCD_DC_DATA();
  (void)LCD_SPI_TransmitBuffer(buffer, (uint32_t)width * (uint32_t)height);
}

void LCD_CopyBufferSwapBytes(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *buffer)
{
  uint16_t row;
  uint16_t col;

  if ((buffer == 0) || (width == 0U) || (height == 0U))
  {
    return;
  }

  if ((x >= LCD_Width) || (y >= LCD_Height))
  {
    return;
  }

  if ((x + width) > LCD_Width)
  {
    width = LCD_Width - x;
  }
  if ((y + height) > LCD_Height)
  {
    height = LCD_Height - y;
  }

  LCD_SetAddress(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  LCD_DC_DATA();

  for (row = 0U; row < height; row++)
  {
    const uint16_t *src = &buffer[(uint32_t)row * (uint32_t)width];
    for (col = 0U; col < width; col++)
    {
      uint16_t pixel = src[col];
      lcd_line_buffer[col] = (uint16_t)((pixel << 8U) | (pixel >> 8U));
    }
    (void)LCD_SPI_TransmitBuffer(lcd_line_buffer, width);
  }
}

void LCD_DisplayString(uint16_t x, uint16_t y, const char *str)
{
  uint16_t xpos = x;

  if (str == 0)
  {
    return;
  }

  while ((*str != '\0') && ((xpos + LCD_CHAR_WIDTH) <= LCD_Width))
  {
    LCD_DrawChar(xpos, y, *str);
    xpos = (uint16_t)(xpos + LCD_CHAR_WIDTH);
    str++;
  }
}

void LCD_DisplayNumber(uint16_t x, uint16_t y, uint32_t number, uint8_t len)
{
  char text[11];
  uint8_t i;

  if (len == 0U)
  {
    return;
  }
  if (len > 10U)
  {
    len = 10U;
  }

  text[len] = '\0';
  for (i = 0U; i < len; i++)
  {
    uint8_t index = (uint8_t)(len - 1U - i);
    text[index] = (char)('0' + (number % 10U));
    number /= 10U;
  }

  LCD_DisplayString(x, y, text);
}

void LCD_DisplayStringTransparent(uint16_t x, uint16_t y, const char *str)
{
  uint16_t xpos = x;

  if (str == 0)
  {
    return;
  }

  while ((*str != '\0') && ((xpos + LCD_CHAR_WIDTH) <= LCD_Width))
  {
    LCD_DrawCharTransparent(xpos, y, *str);
    xpos = (uint16_t)(xpos + LCD_CHAR_WIDTH);
    str++;
  }
}

void LCD_DisplayNumberTransparent(uint16_t x, uint16_t y, uint32_t number, uint8_t len)
{
  char text[11];
  uint8_t i;

  if (len == 0U)
  {
    return;
  }
  if (len > 10U)
  {
    len = 10U;
  }

  text[len] = '\0';
  for (i = 0U; i < len; i++)
  {
    uint8_t index = (uint8_t)(len - 1U - i);
    text[index] = (char)('0' + (number % 10U));
    number /= 10U;
  }

  LCD_DisplayStringTransparent(x, y, text);
}

void LCD_DisplayHexNumber(uint16_t x, uint16_t y, uint32_t number, uint8_t len)
{
  char text[9];
  uint8_t i;

  if (len == 0U)
  {
    return;
  }
  if (len > 8U)
  {
    len = 8U;
  }

  text[len] = '\0';
  for (i = 0U; i < len; i++)
  {
    uint8_t nibble = (uint8_t)(number & 0x0FU);
    uint8_t index = (uint8_t)(len - 1U - i);
    text[index] = (nibble < 10U) ? (char)('0' + nibble) : (char)('A' + nibble - 10U);
    number >>= 4U;
  }

  LCD_DisplayString(x, y, text);
}

void LCD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       uint16_t color, uint8_t thickness)
{
  uint8_t t;

  if ((width == 0U) || (height == 0U) || (thickness == 0U))
  {
    return;
  }

  if ((x >= LCD_Width) || (y >= LCD_Height))
  {
    return;
  }

  if ((x + width) > LCD_Width)
  {
    width = LCD_Width - x;
  }
  if ((y + height) > LCD_Height)
  {
    height = LCD_Height - y;
  }

  for (t = 0U; t < thickness; t++)
  {
    if ((width <= (2U * t)) || (height <= (2U * t)))
    {
      break;
    }

    LCD_DrawHLine((uint16_t)(x + t), (uint16_t)(y + t),
                  (uint16_t)(width - (2U * t)), color);
    LCD_DrawHLine((uint16_t)(x + t), (uint16_t)(y + height - 1U - t),
                  (uint16_t)(width - (2U * t)), color);

    LCD_DrawVLine((uint16_t)(x + t), (uint16_t)(y + t),
                  (uint16_t)(height - (2U * t)), color);
    LCD_DrawVLine((uint16_t)(x + width - 1U - t), (uint16_t)(y + t),
                  (uint16_t)(height - (2U * t)), color);
  }
}

void LCD_DrawCross(uint16_t x, uint16_t y, uint16_t color)
{
  uint16_t x0;
  uint16_t y0;
  uint16_t width;

  if ((x >= LCD_Width) || (y >= LCD_Height))
  {
    return;
  }

  x0 = (x > 2U) ? (uint16_t)(x - 2U) : 0U;
  width = ((x0 + 5U) > LCD_Width) ? (uint16_t)(LCD_Width - x0) : 5U;
  LCD_DrawHLine(x0, y, width, color);

  y0 = (y > 2U) ? (uint16_t)(y - 2U) : 0U;
  LCD_DrawVLine(x, y0, ((y0 + 5U) > LCD_Height) ? (uint16_t)(LCD_Height - y0) : 5U, color);
}

static void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t width, uint16_t color)
{
  uint16_t i;

  if ((x >= LCD_Width) || (y >= LCD_Height) || (width == 0U))
  {
    return;
  }

  if ((x + width) > LCD_Width)
  {
    width = LCD_Width - x;
  }

  for (i = 0U; i < width; i++)
  {
    lcd_line_buffer[i] = color;
  }

  LCD_SetAddress(x, y, (uint16_t)(x + width - 1U), y);
  LCD_DC_DATA();
  (void)LCD_SPI_TransmitBuffer(lcd_line_buffer, width);
}

static void LCD_DrawVLine(uint16_t x, uint16_t y, uint16_t height, uint16_t color)
{
  uint16_t i;

  if ((x >= LCD_Width) || (y >= LCD_Height) || (height == 0U))
  {
    return;
  }

  if ((y + height) > LCD_Height)
  {
    height = LCD_Height - y;
  }

  for (i = 0U; i < height; i++)
  {
    lcd_line_buffer[i] = color;
  }

  LCD_SetAddress(x, y, x, (uint16_t)(y + height - 1U));
  LCD_DC_DATA();
  (void)LCD_SPI_TransmitBuffer(lcd_line_buffer, height);
}

static const uint8_t *LCD_GetGlyph(char ch)
{
  if ((ch >= 'A') && (ch <= 'Z'))
  {
    return glyph_letters[(uint8_t)(ch - 'A')];
  }

  if ((ch >= '0') && (ch <= '9'))
  {
    return glyph_digits[(uint8_t)(ch - '0')];
  }

  switch (ch)
  {
    case ':':
      return glyph_colon;
    case ' ':
    default:
      return glyph_blank;
  }
}

static void LCD_DrawChar(uint16_t x, uint16_t y, char ch)
{
  const uint8_t *glyph = LCD_GetGlyph(ch);
  uint16_t i;
  uint8_t col;
  uint8_t row;

  if ((x >= LCD_Width) || (y >= LCD_Height))
  {
    return;
  }

  for (i = 0U; i < (LCD_CHAR_WIDTH * LCD_CHAR_HEIGHT); i++)
  {
    lcd_char_buffer[i] = lcd_back_color;
  }

  for (col = 0U; col < 5U; col++)
  {
    for (row = 0U; row < 7U; row++)
    {
      if ((glyph[col] & (uint8_t)(1U << row)) != 0U)
      {
        uint8_t dx;
        uint8_t dy;
        uint16_t px = (uint16_t)(1U + ((uint16_t)col * LCD_FONT_SCALE));
        uint16_t py = (uint16_t)(1U + ((uint16_t)row * LCD_FONT_SCALE));

        for (dy = 0U; dy < LCD_FONT_SCALE; dy++)
        {
          for (dx = 0U; dx < LCD_FONT_SCALE; dx++)
          {
            uint16_t bx = (uint16_t)(px + dx);
            uint16_t by = (uint16_t)(py + dy);
            if ((bx < LCD_CHAR_WIDTH) && (by < LCD_CHAR_HEIGHT))
            {
              lcd_char_buffer[(by * LCD_CHAR_WIDTH) + bx] = lcd_pen_color;
            }
          }
        }
      }
    }
  }

  LCD_CopyBuffer(x, y, LCD_CHAR_WIDTH, LCD_CHAR_HEIGHT, lcd_char_buffer);
}

static void LCD_DrawCharTransparent(uint16_t x, uint16_t y, char ch)
{
  const uint8_t *glyph = LCD_GetGlyph(ch);
  uint8_t col;
  uint8_t row;

  if ((x >= LCD_Width) || (y >= LCD_Height))
  {
    return;
  }

  for (col = 0U; col < 5U; col++)
  {
    for (row = 0U; row < 7U; row++)
    {
      if ((glyph[col] & (uint8_t)(1U << row)) != 0U)
      {
        uint8_t dy;
        uint16_t px = (uint16_t)(x + 1U + ((uint16_t)col * LCD_FONT_SCALE));
        uint16_t py = (uint16_t)(y + 1U + ((uint16_t)row * LCD_FONT_SCALE));

        for (dy = 0U; dy < LCD_FONT_SCALE; dy++)
        {
          LCD_DrawHLine(px, (uint16_t)(py + dy), LCD_FONT_SCALE, lcd_pen_color);
        }
      }
    }
  }
}
