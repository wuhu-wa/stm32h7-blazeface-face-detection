#include "usart.h"
#include <stdio.h>

#if defined(__CC_ARM)
#pragma import(__use_no_semihosting)
struct __FILE
{
  int handle;
};
FILE __stdout;
FILE __stdin;

void _sys_exit(int x)
{
  (void)x;
}

void _ttywrch(int ch)
{
  uint8_t c = (uint8_t)ch;
  (void)HAL_UART_Transmit(&huart1, &c, 1U, HAL_MAX_DELAY);
}
#endif

int fputc(int ch, FILE *f)
{
  uint8_t c = (uint8_t)ch;

  (void)f;
  (void)HAL_UART_Transmit(&huart1, &c, 1U, HAL_MAX_DELAY);
  return ch;
}

#if defined(__GNUC__)
int _write(int file, char *ptr, int len)
{
  (void)file;
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
  return len;
}
#endif
