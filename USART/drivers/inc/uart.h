#ifndef UART_H
#define UART_H

#include "stm32h7xx.h"


void UART_Init(USART_TypeDef *USARTx, uint32_t baud);
uint32_t UART_GetPCLK(USART_TypeDef *USARTx);

void UART1_Init(uint32_t baud);
void UART2_Init(uint32_t baud);
void UART3_Init(uint32_t baud);
void UART4_Init(uint32_t baud);
void UART5_Init(uint32_t baud);
void UART6_Init(uint32_t baud);
void UART7_Init(uint32_t baud);
void UART8_Init(uint32_t baud);

void UART_SendChar(USART_TypeDef *UARTx, char c);
void UART_SendString(USART_TypeDef *UARTx, const char *s);
char UART_ReadChar(USART_TypeDef *UARTx);
uint8_t UART_Available(USART_TypeDef *UARTx);

#endif //UART_H
