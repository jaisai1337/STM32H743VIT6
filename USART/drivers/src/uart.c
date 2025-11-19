#include "stm32h7xx.h"
#include "stm32h743xx.h"
#include "uart.h"
#include <stddef.h>


typedef struct {
    USART_TypeDef *Instance;
    GPIO_TypeDef  *TX_Port;
    uint8_t        TX_Pin;
    uint8_t        TX_AF;

    GPIO_TypeDef  *RX_Port;
    uint8_t        RX_Pin;
    uint8_t        RX_AF;

    volatile uint32_t *ClockReg;   // RCC Register
    uint32_t ClockBit;             // Bit mask
    uint8_t APBx;                  // 1 = APB1, 2 = APB2
} UART_Map_t;

__attribute__((weak)) int __io_putchar(int ch)
{
    // Send the character to USART2
    UART_SendChar(USART2, (char)ch);
    return ch;
}
/* UART PIN + CLOCK MAP */
static const UART_Map_t UART_MAP[] = {
    // USART1 → PA9 / PA10 AF7 (APB2)
    {USART1, GPIOA, 9, 7, GPIOA, 10, 7, &RCC->APB2ENR,  RCC_APB2ENR_USART1EN, 2},

    // USART2 → PA2 / PA3 AF7 (APB1)
    {USART2, GPIOA, 2, 7, GPIOA, 3, 7, &RCC->APB1LENR, RCC_APB1LENR_USART2EN, 1},

    // USART3 → PB10 / PB11 AF7 (APB1)
    {USART3, GPIOB, 10, 7, GPIOB, 11, 7, &RCC->APB1LENR, RCC_APB1LENR_USART3EN, 1},

    // UART4 → PA0 / PA1 AF8 (APB1)
    {UART4, GPIOA, 0, 8, GPIOA, 1, 8, &RCC->APB1LENR, RCC_APB1LENR_UART4EN, 1},

    // UART5 → PC12 / PD2 AF8 (APB1)
    {UART5, GPIOC, 12,8, GPIOD, 2, 8, &RCC->APB1LENR, RCC_APB1LENR_UART5EN, 1},

    // USART6 → PC6 / PC7 AF7 (APB2)
    {USART6, GPIOC, 6, 7, GPIOC, 7, 7, &RCC->APB2ENR, RCC_APB2ENR_USART6EN, 2},

    // UART7 → PE8 / PE7 AF7 (APB1)
    {UART7, GPIOE, 8, 7, GPIOE, 7, 7, &RCC->APB1LENR, RCC_APB1LENR_UART7EN, 1},

    // UART8 → PE1 / PE0 AF8 (APB1)
    {UART8, GPIOE, 1, 8, GPIOE, 0, 8, &RCC->APB1LENR, RCC_APB1LENR_UART8EN, 1},
};
static void uart_pin_setup(GPIO_TypeDef *Port, uint8_t Pin, uint8_t AF)
{
    // Enable GPIO Clock
    if (Port == GPIOA) RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    if (Port == GPIOB) RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    if (Port == GPIOC) RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    if (Port == GPIOD) RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    if (Port == GPIOE) RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;

    // AF mode
    Port->MODER &= ~(3U << (Pin*2));
    Port->MODER |=  (2U << (Pin*2));

    if (Pin < 8) {
        Port->AFR[0] &= ~(0xF << (Pin*4));
        Port->AFR[0] |=  (AF  << (Pin*4));
    } else {
        uint8_t sh = (Pin - 8) * 4;
        Port->AFR[1] &= ~(0xF << sh);
        Port->AFR[1] |=  (AF  << sh);
    }

    Port->OTYPER &= ~(1 << Pin);
    Port->PUPDR  &= ~(3 << (Pin*2));
    Port->OSPEEDR |= (3 << (Pin*2));   // Very High speed
}
void UART_Init(USART_TypeDef *USARTx, uint32_t baud)
{
    const UART_Map_t *m = NULL;

    // Find entry in lookup table
    for (uint32_t i = 0; i < (sizeof(UART_MAP)/sizeof(UART_MAP[0])); i++){
        if (UART_MAP[i].Instance == USARTx) {
            m = &UART_MAP[i];
            break;
        }
    }
    if (!m) return; // Invalid UART

    // ----- Enable UART Peripheral Clock -----
    *(m->ClockReg) |= m->ClockBit;

    // ----- Configure Pins -----
    uart_pin_setup(m->TX_Port, m->TX_Pin, m->TX_AF);
    uart_pin_setup(m->RX_Port, m->RX_Pin, m->RX_AF);

    // ----- Baudrate -----
    // uint32_t pclk = SystemCoreClock;
    // uint32_t pclk = 120000000;
    uint32_t pclk = UART_GetPCLK(USARTx);

    USARTx->CR1 &= ~USART_CR1_UE;
    USARTx->CR1 &= ~USART_CR1_FIFOEN;   // <-- ADD THIS
    USARTx->BRR = pclk / baud;

    // Enable TX + RX + UART
    USARTx->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USARTx->CR1 |= USART_CR1_UE;

    while (!(USARTx->ISR & USART_ISR_TEACK));
    while (!(USARTx->ISR & USART_ISR_REACK));
}

uint32_t UART_GetPCLK(USART_TypeDef *USARTx)
{
    uint32_t pclk1, pclk2;
    // --- Determine clock setup based on SystemCoreClock ---
    if (SystemCoreClock == 480000000)       // High-speed configuration
    {
        pclk1 = 120000000;  // APB1 at 120 MHz
        pclk2 = 120000000;  // APB2 at 120 MHz
    }
    else if (SystemCoreClock == 64000000)   // Low-speed configuration
    {
        pclk1 = 64000000;   // APB1 = 64 MHz
        pclk2 = 64000000;   // APB2 = 64 MHz
    }
    else
    { return 0;} // Unsupported clock
    // APB2 USARTs
    if (USARTx == USART1 || USARTx == USART6)
        return pclk2;

    // APB1 USARTs / UARTs
    if (USARTx == USART2 || USARTx == USART3 ||
        USARTx == UART4  || USARTx == UART5 ||
        USARTx == UART7  || USARTx == UART8)
        return pclk1;
    return 0;
}



#if 1
void UART1_Init(uint32_t baud)
{
    RCC->AHB4ENR  |= RCC_AHB4ENR_GPIOAEN;
    RCC->APB2ENR  |= RCC_APB2ENR_USART1EN;   // USART1 is on APB2
    GPIOA->MODER &= ~((3 << (9*2)) | (3 << (10*2)));
    GPIOA->MODER |=  ((2 << (9*2)) | (2 << (10*2)));    // AF mode
    GPIOA->AFR[1] &= ~((0xF << ((9-8)*4)) | (0xF << ((10-8)*4)));
    GPIOA->AFR[1] |=  ((7 << ((9-8)*4)) | (7 << ((10-8)*4))); // AF7
    GPIOA->OTYPER  &= ~((1 << 9) | (1 << 10));
    GPIOA->PUPDR   &= ~((3 << (9*2)) | (3 << (10*2)));
    GPIOA->OSPEEDR |=  ((3 << (9*2)) | (3 << (10*2)));
    USART1->CR1 &= ~USART_CR1_UE; // Disable USART before config
    USART1->BRR = SystemCoreClock / baud;  // Baudrate
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;  // Enable TX & RX
    USART1->CR1 |= USART_CR1_UE;                 // Enable USART
    while(!(USART1->ISR & USART_ISR_TEACK));
    while(!(USART1->ISR & USART_ISR_REACK));
}
void UART2_Init(uint32_t baud)
{
    RCC->AHB4ENR   |= RCC_AHB4ENR_GPIOAEN;
    RCC->APB1LENR  |= RCC_APB1LENR_USART2EN;
    GPIOA->MODER &= ~((3 << (2*2)) | (3 << (3*2)));
    GPIOA->MODER |=  ((2 << (2*2)) | (2 << (3*2)));
    GPIOA->AFR[0] &= ~((0xF << (2*4)) | (0xF << (3*4)));
    GPIOA->AFR[0] |=  ((7 << (2*4)) | (7 << (3*4)));
    GPIOA->OSPEEDR |= ((3 << (2*2)) | (3 << (3*2))); // High speed
    GPIOA->OTYPER  &= ~((1 << 2) | (1 << 3));        // Push-pull
    GPIOA->PUPDR   &= ~((3 << (2*2)) | (3 << (3*2))); // No Pull
    USART2->CR1 &= ~USART_CR1_UE; // Disable USART to configure
    USART2->BRR = SystemCoreClock / baud;  // Baudrate
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE; // Enable TX + RX
    USART2->CR1 |= USART_CR1_UE; // Enable USART
    while(!(USART2->ISR & USART_ISR_TEACK));
    while(!(USART2->ISR & USART_ISR_REACK));
}
void UART3_Init(uint32_t baud)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    GPIOB->MODER &= ~((3 << (10*2)) | (3 << (11*2)));
    GPIOB->MODER |=  ((2 << (10*2)) | (2 << (11*2)));  // AF mode
    GPIOB->AFR[1] &= ~((0xF << ((10-8)*4)) | (0xF << ((11-8)*4)));
    GPIOB->AFR[1] |=  ((7 << ((10-8)*4)) | (7 << ((11-8)*4)));
    GPIOB->OSPEEDR |= ((3 << (10*2)) | (3 << (11*2)));  // High speed
    GPIOB->OTYPER &= ~((1 << 10) | (1 << 11));          // Push-pull
    GPIOB->PUPDR  &= ~((3 << (10*2)) | (3 << (11*2)));  // No Pull
    USART3->CR1 &= ~USART_CR1_UE; // Disable USART to configure
    USART3->BRR = SystemCoreClock / baud;     // Set baudrate
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE;  // Enable TX + RX
    USART3->CR1 |= USART_CR1_UE;   // Enable USART
    while(!(USART3->ISR & USART_ISR_TEACK));
    while(!(USART3->ISR & USART_ISR_REACK));
}
void UART4_Init(uint32_t baud)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC->APB1LENR |= RCC_APB1LENR_UART4EN;
    GPIOA->MODER &= ~((3 << (0*2)) | (3 << (1*2))); // Clear bits
    GPIOA->MODER |=  ((2 << (0*2)) | (2 << (1*2))); // AF mode
    GPIOA->AFR[0] &= ~((0xF << (0*4)) | (0xF << (1*4)));
    GPIOA->AFR[0] |=  ((8 << (0*4)) | (8 << (1*4)));
    GPIOA->OSPEEDR |= ((3 << (0*2)) | (3 << (1*2))); // High speed
    GPIOA->OTYPER  &= ~((1 << 0) | (1 << 1));        // Push-pull
    GPIOA->PUPDR   &= ~((3 << (0*2)) | (3 << (1*2))); // No pull
    UART4->CR1 &= ~USART_CR1_UE;
    UART4->BRR = SystemCoreClock / baud;
    UART4->CR1 |= USART_CR1_TE | USART_CR1_RE; // TX + RX
    UART4->CR1 |= USART_CR1_UE;                // Enable
    while (!(UART4->ISR & USART_ISR_TEACK));
    while (!(UART4->ISR & USART_ISR_REACK));
}
void UART5_Init(uint32_t baud)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN;
    RCC->APB1LENR |= RCC_APB1LENR_UART5EN;
    GPIOC->MODER &= ~(3 << (12 * 2));
    GPIOC->MODER |=  (2 << (12 * 2)); // AF mode
    GPIOC->AFR[1] &= ~(0xF << ((12 - 8) * 4));
    GPIOC->AFR[1] |=  (8   << ((12 - 8) * 4));  // AF8
    GPIOC->OSPEEDR |= (3 << (12 * 2));
    GPIOC->OTYPER  &= ~(1 << 12);
    GPIOC->PUPDR   &= ~(3 << (12 * 2));
    GPIOD->MODER &= ~(3 << (2 * 2));
    GPIOD->MODER |=  (2 << (2 * 2)); // AF
    GPIOD->AFR[0] &= ~(0xF << (2 * 4));
    GPIOD->AFR[0] |=  (8   << (2 * 4));  // AF8
    GPIOD->OSPEEDR |= (3 << (2 * 2));
    GPIOD->OTYPER  &= ~(1 << 2);
    GPIOD->PUPDR   &= ~(3 << (2 * 2));
    UART5->CR1 &= ~USART_CR1_UE;
    UART5->BRR = SystemCoreClock / baud;
    UART5->CR1 |= USART_CR1_TE | USART_CR1_RE;
    UART5->CR1 |= USART_CR1_UE;
    while (!(UART5->ISR & USART_ISR_TEACK));
    while (!(UART5->ISR & USART_ISR_REACK));
}
void UART6_Init(uint32_t baud)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    GPIOC->MODER &= ~((3 << (6*2)) | (3 << (7*2)));  // Clear mode
    GPIOC->MODER |=  ((2 << (6*2)) | (2 << (7*2)));  // AF mode
    GPIOC->AFR[0] &= ~((0xF << (6*4)) | (0xF << (7*4)));
    GPIOC->AFR[0] |=  ((7 << (6*4)) | (7 << (7*4))); // AF7
    GPIOC->OSPEEDR |= (3 << (6*2)) | (3 << (7*2));
    GPIOC->OTYPER &= ~((1 << 6) | (1 << 7));
    GPIOC->PUPDR  &= ~((3 << (6*2)) | (3 << (7*2)));  // No Pull
    USART6->CR1 &= ~USART_CR1_UE;
    uint32_t pclk2 = 120000000;    // APB2 freq (default)
    USART6->BRR = pclk2 / baud;
    USART6->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART6->CR1 |= USART_CR1_UE;
    while(!(USART6->ISR & USART_ISR_TEACK));
    while(!(USART6->ISR & USART_ISR_REACK));
}
void UART7_Init(uint32_t baud)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    RCC->APB1LENR |= RCC_APB1LENR_UART7EN;
    GPIOE->MODER &= ~((3 << (8*2)) | (3 << (7*2)));
    GPIOE->MODER |=  ((2 << (8*2)) | (2 << (7*2)));  // AF mode
    GPIOE->AFR[0] &= ~(0xF << (7 * 4));
    GPIOE->AFR[0] |=  (7 << (7 * 4));
    GPIOE->AFR[1] &= ~(0xF << ((8 - 8) * 4));
    GPIOE->AFR[1] |=  (7 << ((8 - 8) * 4));
    GPIOE->OSPEEDR |= (3 << (8*2)) | (3 << (7*2));
    GPIOE->OTYPER &= ~((1 << 8) | (1 << 7));
    GPIOE->PUPDR  &= ~((3 << (8*2)) | (3 << (7*2)));
    UART7->CR1 &= ~USART_CR1_UE;
    uint32_t pclk1 = SystemCoreClock; // or APB1 clock if different
    UART7->BRR = pclk1 / baud;
    UART7->CR1 |= USART_CR1_TE | USART_CR1_RE;
    UART7->CR1 |= USART_CR1_UE;
    while(!(UART7->ISR & USART_ISR_TEACK));
    while(!(UART7->ISR & USART_ISR_REACK));
}
void UART8_Init(uint32_t baud)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    RCC->APB1LENR |= RCC_APB1LENR_UART8EN;
    GPIOE->MODER &= ~((3 << (1*2)) | (3 << (0*2)));
    GPIOE->MODER |=  ((2 << (1*2)) | (2 << (0*2)));  // AF mode
    GPIOE->AFR[0] &= ~((0xF << (1*4)) | (0xF << (0*4)));
    GPIOE->AFR[0] |=  ((8 << (1*4)) | (8 << (0*4))); // AF8
    GPIOE->OSPEEDR |= (3 << (1*2)) | (3 << (0*2));
    GPIOE->OTYPER &= ~((1 << 1) | (1 << 0));
    GPIOE->PUPDR  &= ~((3 << (1*2)) | (3 << (0*2))); // No Pull
    UART8->CR1 &= ~USART_CR1_UE;
    uint32_t pclk1 = SystemCoreClock;
    UART8->BRR = pclk1 / baud;
    UART8->CR1 |= USART_CR1_TE | USART_CR1_RE;
    UART8->CR1 |= USART_CR1_UE;
    while(!(UART8->ISR & USART_ISR_TEACK));
    while(!(UART8->ISR & USART_ISR_REACK));
}
#endif


void UART_SendChar(USART_TypeDef *UARTx, char c)
{
    while(!(UARTx->ISR & USART_ISR_TXE_TXFNF));
    UARTx->TDR = c;
}

void UART_SendString(USART_TypeDef *UARTx, const char *s)
{
    while(*s)
    {
        UART_SendChar(UARTx, *s++);
    }
}

char UART_ReadChar(USART_TypeDef *UARTx)
{
    while(!(UARTx->ISR & USART_ISR_RXNE_RXFNE));
    return UARTx->RDR;
}

uint8_t UART_Available(USART_TypeDef *UARTx)
{
    return (UARTx->ISR & USART_ISR_RXNE_RXFNE) != 0;
}