#include "stm32h7xx.h"
#include "stm32h743xx.h"
#include "uart.h"
#include "systick.h"
#include "system_clock.h"
#include <stdio.h>

USART_TypeDef *UARTx = USART2;
int main(void)
{
    SystemClock_Config(); // Configure system clock to 480 MHz
    UART_Init(UARTx, 115200);
    SysTick_Init();
    delay_ms(100);
    UART_SendString(UARTx, "UART2 Ready on PA2/PA3!\r\n");
    printf("SystemCoreClock: %lu Hz\r\n", SystemCoreClock);
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    GPIOE->MODER &= ~(3 << (3*2));
    GPIOE->MODER |=  (1 << (3*2));

    while(1)
    {
        if (UART_Available(UARTx))
        {
            char c = UART_ReadChar(UARTx);
            UART_SendChar(UARTx, c);  // Echo back
            GPIOE->ODR ^= GPIO_ODR_OD3;
        }
    }
}
