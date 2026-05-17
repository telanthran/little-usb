#include "stm32f407_discovery.h"
#include "peripherals/lis3dsh.h"
#include "stm32f4xx.h"


static uint8_t spi_transfer(uint8_t data)
{
    while (!READ_BIT(SPI1->SR, SPI_SR_TXE));
    *((__IO uint8_t *)&SPI1->DR) = data;
    while (!READ_BIT(SPI1->SR, SPI_SR_RXNE));
    return *((__IO uint8_t *)&SPI1->DR);
}

static void cs_assert(void)
{
    CLEAR_BIT(GPIOE->ODR, GPIO_ODR_OD3); 
}

static void cs_deassert(void)
{
    SET_BIT(GPIOE->ODR, GPIO_ODR_OD3);
}

static void button_init(void)
{
    CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MODER0);
}

static uint8_t button_is_pressed(void)
{
    return READ_BIT(GPIOA->IDR, GPIO_IDR_ID0) ? 1 : 0;
}

static ButtonDriver _button_driver = {
    .init       = button_init,
    .is_pressed = button_is_pressed,
};

AccelerometerDriver *board_accel  = &lis3dsh_driver;
ButtonDriver        *board_button = &_button_driver;

void board_init(void)
{
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN);
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);

    // PA5=SCK, PA6=MISO, PA7=MOSI - AF5
    MODIFY_REG(GPIOA->AFR[0],
        GPIO_AFRL_AFSEL5 | GPIO_AFRL_AFSEL6 | GPIO_AFRL_AFSEL7,
        _VAL2FLD(GPIO_AFRL_AFSEL5, 5) | _VAL2FLD(GPIO_AFRL_AFSEL6, 5) | _VAL2FLD(GPIO_AFRL_AFSEL7, 5)
    );
    MODIFY_REG(GPIOA->MODER,
        GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7,
        _VAL2FLD(GPIO_MODER_MODER5, 2) | _VAL2FLD(GPIO_MODER_MODER6, 2) | _VAL2FLD(GPIO_MODER_MODER7, 2)
    );

    // PE3 - CS output, start deasserted
    MODIFY_REG(GPIOE->MODER, GPIO_MODER_MODER3, _VAL2FLD(GPIO_MODER_MODER3, 1));
    SET_BIT(GPIOE->ODR, GPIO_ODR_OD3);

    // SPI1: /32 prescaler, master, software NSS
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, _VAL2FLD(SPI_CR1_BR, 3));
    SET_BIT(SPI1->CR1, SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    lis3dsh_configure((Lis3dshConfig){
        .transfer    = spi_transfer,
        .cs_assert   = cs_assert,
        .cs_deassert = cs_deassert,
    });
}
