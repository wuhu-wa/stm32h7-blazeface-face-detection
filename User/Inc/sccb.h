#ifndef __SCCB_H__
#define __SCCB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define SCCB_SCL_GPIO_Port GPIOB
#define SCCB_SCL_Pin       GPIO_PIN_8
#define SCCB_SDA_GPIO_Port GPIOB
#define SCCB_SDA_Pin       GPIO_PIN_9

#define SCCB_DelayVaule    8U
#define SCCB_OV2640_ADDR   0x60U

extern volatile uint8_t SCCB_LastAckError;
extern volatile uint8_t SCCB_LastRegAddr;

void SCCB_GPIO_Config(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_ACK(void);
void SCCB_NoACK(void);
uint8_t SCCB_WaitACK(void);
uint8_t SCCB_WriteByte(uint8_t data);
uint8_t SCCB_ReadByte(void);
uint8_t SCCB_WriteReg(uint8_t addr, uint8_t value);
uint8_t SCCB_ReadReg(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __SCCB_H__ */
