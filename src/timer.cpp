#include "stm32h743xx.h"
#include "timer.hpp"

namespace timer{

    //Initialize the TIM1
    void init(){
        //Enbale the timer peripheral clock
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

        //Enable PA8 as TIM1_CH1 output 
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
        GPIOA->MODER |= (2 << GPIO_MODER_MODE8_Pos);
        GPIOA->OSPEEDR |= (3 << GPIO_OSPEEDR_OSPEED8_Pos);
        GPIOA->AFR[1] |= (1 << GPIO_AFRH_AFSEL8_Pos);

        //Counting at 80MHZ 50% duty
        TIM1->PSC = 0;
        TIM1->ARR = 1;
        TIM1->CCR1 = 1;

        //Enable OC PWM mode
        TIM1->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos);
        //Enable preload (maybe not)
        TIM1->CCMR1 |= (1 << TIM_CCMR1_OC1PE_Pos);
        TIM1->CR1 |= (1 << TIM_CR1_ARPE_Pos);

        TIM1->CCER &= ~TIM_CCER_CC1P;
        TIM1->CCER |= TIM_CCER_CC1E;

        //Interrupt only on overflow/underflow
        TIM1->CR1 |= (1 << TIM_CR1_URS_Pos);
    
        //Load the registers to the shadow ones
        TIM1->EGR |= TIM_EGR_UG;


        ///
        ///Trigger setup
        //Select T2 as trigger
        TIM1->SMCR |= (0b00110 << TIM_SMCR_TS_Pos);
        //Slave mode = reset + trigger
        TIM1->SMCR |= (0b1000 << TIM_SMCR_SMS_Pos);

        //Prescale the sync signal
        TIM1->SMCR |= (0b11 << TIM_SMCR_ETPS_Pos);

        //Set TI2 as input for CC2 channel
        TIM1->CCMR1 |= (0b01 << TIM_CCMR1_CC2S_Pos);
        TIM1->CCER &= ~TIM_CCER_CC2P;
        TIM1->CCER &= ~TIM_CCER_CC2NP;


        //Enable the timer
        TIM1->CR1 |= (1 << TIM_CR1_CEN_Pos);

    }

    
}
