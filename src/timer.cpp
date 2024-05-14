#include "stm32h753xx.h"
#include "timer.hpp"

namespace timer{

    //Initialize the TIM1
    void init(){
        //Enbale the clocks
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;

        //Enable PA8 as TIM1_CH1 output 
        GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE8_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE8_Pos);
        GPIOA->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED8_Pos);
        GPIOA->AFR[1] = (1 << GPIO_AFRH_AFSEL8_Pos);

        //Enable PA7 as TIM1_CH1N output 
        GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE7_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE7_Pos);
        GPIOA->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED7_Pos);
        GPIOA->AFR[0] = (1 << GPIO_AFRL_AFSEL7_Pos);

        //Enable PA9 as TIM1_CH2 <- sync input
       /* GPIOA->MODER &= ~(0b11 << GPIO_MODER_MODE9_Pos);
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODE9_Pos);
        GPIOA->AFR[1] |= (1 << GPIO_AFRH_AFSEL9_Pos);
        GPIOA->PUPDR |= (0b10 << GPIO_PUPDR_PUPD9_Pos);*/

        //Counting at 80MHZ 50% duty
        TIM1->PSC = 0;
        TIM1->ARR = 1;
        TIM1->CCR1 = 1;

        //Enable OC PWM mode
        TIM1->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos);
        TIM1->CCMR1 |= (1 << TIM_CCMR1_OC1PE_Pos);
        //Enable preload (maybe not)
        //TIM1->CR1 |= (1 << TIM_CR1_ARPE_Pos);


        //Enable complementary output
        TIM1->CCER |= TIM_CCER_CC1NE;

        //Enable normal output
        TIM1->CCER |= TIM_CCER_CC1E;

        //Interrupt only on overflow/underflow                                         
        TIM1->CR1 |= (1 << TIM_CR1_URS_Pos);
    
        //Load the registers to the shadow ones
        TIM1->EGR |= TIM_EGR_UG;

        ///Trigger setup
        //Select TI2FP2 as trigger
        TIM1->SMCR |= (0b00110 << TIM_SMCR_TS_Pos);
        //Slave mode = reset + trigger
        TIM1->SMCR |= (0b100 << TIM_SMCR_SMS_Pos);//1000
        //TIM1->SMCR |= (TIM_SMCR_SMS_3);//1000

        //Prescale the sync signal
        //TIM1->SMCR |= (0b11 << TIM_SMCR_ETPS_Pos);

        //Enable the timer  
        TIM1->CR1 |= (1 << TIM_CR1_CEN_Pos);
        
        //Enable output
        TIM1->BDTR |= TIM_BDTR_MOE;
    }

    
}
