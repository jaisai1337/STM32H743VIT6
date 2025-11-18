#include "stm32h7xx.h"
#include "core_cm7.h"

volatile uint32_t systick_ms = 0;

void SysTick_Handler(void)
{
    systick_ms++;
}

void SysTick_Init(void)
{
    // SysTick = 1 ms
    SysTick->LOAD = (480000 - 1);     // 64 MHz / 1000 = 64000
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms);
}
