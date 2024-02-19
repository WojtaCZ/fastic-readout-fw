#include "status.hpp"
#include "stm32h753xx.h"


namespace status{

    bool error_ = false;

    void init(){
        //Enable gpio clock
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
        //Setup led pins to be outputs
        GPIOB->MODER &= ~((0b11 << GPIO_MODER_MODE0_Pos) | (0b11 << GPIO_MODER_MODE7_Pos) | (0b11 << GPIO_MODER_MODE14_Pos));
        GPIOB->MODER |= (1 << GPIO_MODER_MODE0_Pos) | (1 << GPIO_MODER_MODE7_Pos) | (1 << GPIO_MODER_MODE14_Pos);
    }

    void process(){
        if(error_){
            GPIOB->BSRR = GPIO_BSRR_BS14;
            GPIOB->BSRR = GPIO_BSRR_BR0;
        }else if(!error_){
            GPIOB->BSRR = GPIO_BSRR_BR14;
            GPIOB->BSRR = GPIO_BSRR_BS0;
        }
    }

    namespace error{
        void set(){
            error_ = true;
        }

        void reset(){
            error_ = false;
        }
    }
}