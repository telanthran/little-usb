#include <stdint.h>
#include "system_stm32f4xx.h"
#include "stm32f4xx.h"
#include "logger.h"

LogLevel system_log_level = LOG_LEVEL_DEBUG;
uint32_t SystemCoreClock = 168000000;

// M = 8
// N = x336
// P = 4
// Q = 7
static void configure_clock(void)
{
    MODIFY_REG(FLASH->ACR,
               FLASH_ACR_LATENCY,
               _VAL2FLD(FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_2WS)
               // FLASH_ACR_LATENCY_2WS << FLASH_ACR_LATENCY_Pos
               );

    // Enable HSE
    SET_BIT(RCC->CR, RCC_CR_HSEON);

    // Wait for HSE to stabilize
    while(!READ_BIT(RCC->CR, RCC_CR_HSERDY));

    MODIFY_REG(RCC->PLLCFGR,
               RCC_PLLCFGR_PLLM,
               _VAL2FLD(RCC_PLLCFGR_PLLM, 0x8));
    MODIFY_REG(RCC->PLLCFGR,
               RCC_PLLCFGR_PLLN,
               _VAL2FLD(RCC_PLLCFGR_PLLN, 336));
    MODIFY_REG(RCC->PLLCFGR,
               RCC_PLLCFGR_PLLP,
               _VAL2FLD(RCC_PLLCFGR_PLLP, 0b01));
    // Adjust PLL factor to /7 before enabling it
    MODIFY_REG(RCC->PLLCFGR,
               RCC_PLLCFGR_PLLQ,
               _VAL2FLD(RCC_PLLCFGR_PLLQ, 0x7));
    MODIFY_REG(RCC->PLLCFGR,
               RCC_PLLCFGR_PLLSRC,
               RCC_PLLCFGR_PLLSRC_HSE);

    // Enable PLL
    SET_BIT(RCC->CR, RCC_CR_PLLON);

    while(!READ_BIT(RCC->CR, RCC_CR_PLLRDY));

    // Switch system clock to PLL
    MODIFY_REG(RCC->CFGR,
               RCC_CFGR_SW,
               _VAL2FLD(RCC_CFGR_SW, RCC_CFGR_SW_PLL));

    // Configure APB Prescaler 1 (PPRE1) to be /4
    MODIFY_REG(RCC->CFGR,
               RCC_CFGR_PPRE1,
               _VAL2FLD(RCC_CFGR_PPRE1, 0b101));

    // Configure APB Prescaler 2 (PPRE2) to be /2
    MODIFY_REG(RCC->CFGR,
               RCC_CFGR_PPRE2,
               _VAL2FLD(RCC_CFGR_PPRE2, 0b100));

    // Wait until PLL is used
    while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // Disable HSI
    CLEAR_BIT(RCC->CFGR, RCC_CR_HSION);

}

void configure_mco1(void)
{
    // Configure MCO1: source = PLL
    MODIFY_REG(RCC->CFGR,
               RCC_CFGR_MCO1,
               _VAL2FLD(RCC_CFGR_MCO1, 0b11));
    // Configure MCO1 prescaler: /2 = 0b100, /1 = 0b000
    MODIFY_REG(RCC->CFGR,
               RCC_CFGR_MCO1PRE,
               _VAL2FLD(RCC_CFGR_MCO1PRE, 0b000));

    // Enable GPIOA port
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);

    // Configure PA8 as med speed
    MODIFY_REG(GPIOA->OSPEEDR,
               GPIO_OSPEEDR_OSPEED8,
               _VAL2FLD(GPIO_OSPEEDR_OSPEED8, 1));

    // Configure PA8 to work in AF mode
    MODIFY_REG(GPIOA->MODER,
               GPIO_MODER_MODER8,
               _VAL2FLD(GPIO_MODER_MODER8, 2));
}

void SystemInit(void)
{
    SCB->CPACR |= ((3UL << 20) | (3UL << 22));  // enable CP10 and CP11 (FPU full access)
    configure_mco1();
    configure_clock();
}
