#include "stm32h7xx.h"
#include "stm32h743xx.h"
#include "system_clock.h"



void SystemClock_Config(void)
{
    /* 1. Power Supply Configuration */
    // Enable LDO by setting LDOEN bit in PWR_CR3
    PWR->CR3 |= PWR_CR3_LDOEN;
    // Wait for regulator to be ready
    while (!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY));

    /* 2. Voltage Scaling Configuration (Scale 0) */
    // Enable SYSCFG clock (required to control Overdrive in SYSCFG->PWRCR)
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;

    // Set VOS to Scale 1 (0b11) first. This is required before enabling Overdrive (Scale 0).
    // Note: PWR_D3CR_VOS (0xC000) sets both bits 14 and 15 to 1.
    PWR->D3CR |= PWR_D3CR_VOS; 
    while (!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY));

    // Enable Overdrive mode to reach Scale 0 (Effective for 480 MHz)
    SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
    while (!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY));

    /* 3. Oscillator Configuration (HSI) */
    // Enable HSI
    RCC->CR |= RCC_CR_HSION;
    // Wait until HSI is ready
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Set HSI Divider to 1
    RCC->CR &= ~RCC_CR_HSIDIV;       // Clear division field
    RCC->CR |= RCC_CR_HSIDIV_1;      // Set to 1 (though usually 0 means div1, check specific header)

    // Calibration (Trimming)
    RCC->HSICFGR = (RCC->HSICFGR & ~RCC_HSICFGR_HSITRIM_Msk) | (0x40U << RCC_HSICFGR_HSITRIM_Pos);

    /* 4. PLL1 Configuration */
    // Disable PLL1 before updating parameters
    RCC->CR &= ~RCC_CR_PLL1ON;
    while (RCC->CR & RCC_CR_PLL1RDY);

    // Set PLL Source to HSI
    RCC->PLLCKSELR = (RCC->PLLCKSELR & ~RCC_PLLCKSELR_PLLSRC_Msk) | RCC_PLLCKSELR_PLLSRC_HSI;

    // Configure Divider M = 4
    RCC->PLLCKSELR = (RCC->PLLCKSELR & ~RCC_PLLCKSELR_DIVM1_Msk) | (4U << RCC_PLLCKSELR_DIVM1_Pos);

    // Configure Multiplier N = 60
    // Value written is N - 1
    RCC->PLL1DIVR = (RCC->PLL1DIVR & ~RCC_PLL1DIVR_N1_Msk) | ((60U - 1U) << RCC_PLL1DIVR_N1_Pos);

    // Configure Divider P = 2 (System Clock)
    // Value written is P - 1
    RCC->PLL1DIVR = (RCC->PLL1DIVR & ~RCC_PLL1DIVR_P1_Msk) | ((2U - 1U) << RCC_PLL1DIVR_P1_Pos);

    // Configure Divider Q = 2 (Peripheral Clock)
    RCC->PLL1DIVR = (RCC->PLL1DIVR & ~RCC_PLL1DIVR_Q1_Msk) | ((2U - 1U) << RCC_PLL1DIVR_Q1_Pos);

    // Configure Divider R = 2 (Peripheral Clock)
    RCC->PLL1DIVR = (RCC->PLL1DIVR & ~RCC_PLL1DIVR_R1_Msk) | ((2U - 1U) << RCC_PLL1DIVR_R1_Pos);

    // PLL Range and VCO Settings
    // Range 3 (Input 8-16 MHz, HSI/4 = 16MHz)
    RCC->PLLCFGR = (RCC->PLLCFGR & ~RCC_PLLCFGR_PLL1RGE_Msk) | RCC_PLLCFGR_PLL1RGE_3;
    
    // Wide VCO Range, Disable Fractional Mode
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1VCOSEL;
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1FRACEN;
    RCC->PLL1FRACR = 0;

    // Enable PLL1
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    /* 5. Bus Prescalers Configuration */
    // D1 Domain (Core & AHB)
    // HPRE (AHB) = DIV2, D1CPRE (Core) = DIV1, D1PPRE (APB3) = DIV2
    RCC->D1CFGR = (RCC_D1CFGR_HPRE_DIV2 | RCC_D1CFGR_D1CPRE_DIV1 | RCC_D1CFGR_D1PPRE_DIV2);

    // D2 Domain (APB1 & APB2)
    // D2PPRE1 (APB1) = DIV2, D2PPRE2 (APB2) = DIV2
    RCC->D2CFGR = (RCC_D2CFGR_D2PPRE1_DIV2 | RCC_D2CFGR_D2PPRE2_DIV2);

    // D3 Domain (APB4)
    // D3PPRE = DIV2
    RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2;

    /* 6. Flash Latency */
    // For 480MHz at VOS0, we need appropriate wait states (Latency 4)
    // Note: Latency must be set BEFORE increasing clock frequency
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | FLASH_ACR_LATENCY_4WS;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_4WS);

    /* 7. System Clock Switch */
    // Select PLL1 as system clock source
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL1;

    // Wait until PLL1 is used as system clock source
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL1);
    // SystemCoreClockUpdate();
}



int main(void)
{
    SystemClock_Config();
    SysTick_Init();
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    GPIOE->MODER &= ~(3 << (3*2));
    GPIOE->MODER |=  (1 << (3*2));


    while (1) {
        GPIOE->ODR ^= GPIO_ODR_OD3;
        delay_ms(100);
    }
}
