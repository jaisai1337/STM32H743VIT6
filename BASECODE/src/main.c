#include "stm32h7xx.h"
#include "system_clock.h"

int main(void)
{
    SystemClock_Config();
    SysTick_Init();
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    (void)RCC->AHB4ENR; // Dummy read to ensure clock is active before accessing GPIOE
    GPIOE->MODER &= ~(3 << (3*2));
    GPIOE->MODER |=  (1 << (3*2));


    while (1) {
        GPIOE->ODR ^= GPIO_ODR_OD3;
        delay_ms(1000);
    }
}
