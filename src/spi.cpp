#include "spi.hpp"
#include "stm32h753xx.h"

/*

PA5 (SPI1_SCK) connected to PA7 (TIM1_CH1N)


*/

namespace spi{
    void init(){
        RCC->APB1LENR |= (1 << RCC_APB1LENR_SPI2EN_Pos);
        RCC->APB2ENR |= (1 << RCC_APB2ENR_SPI1EN_Pos);

        //Enable PA5 as SPI1_SCK
        GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE5_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE5_Pos);
        GPIOA->AFR[0] = (5 << GPIO_AFRL_AFSEL5_Pos);

        //Enable PA12 as SPI2_SCK
        GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE12_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE12_Pos);
        GPIOA->AFR[1] = (1 << GPIO_AFRH_AFSEL12_Pos);

        //Enable PB14 as SPI2_MOSI
        GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE14_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE14_Pos);
        GPIOA->AFR[1] = (5 << GPIO_AFRH_AFSEL14_Pos);

        //Enable PB5 as SPI1_MOSI
        GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE5_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE5_Pos);
        GPIOA->AFR[0] = (1 << GPIO_AFRL_AFSEL5_Pos);

        //32bit data words, RX dma enabled
        SPI1->CFG1 |= (1 << SPI_CFG1_RXDMAEN_Pos) | (0b11111 << SPI_CFG1_DSIZE_Pos) | (0b0000 << SPI_CFG1_FTHLV_Pos);
        //SS level determined by SSI bit not pin, simplex receiver 
        SPI1->CFG2 |= (1 << SPI_CFG2_SSM_Pos) | (0b10 << SPI_CFG2_COMM_Pos);

        //Enable the SPI peripherals
        SPI1->CR1 |= (1 << SPI_CR1_SPE_Pos);
        SPI2->CR1 |= (1 << SPI_CR1_SPE_Pos);
    }
}