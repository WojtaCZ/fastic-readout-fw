#include "i2c.hpp"
#include "stm32h753xx.h"

#include "gpio.hpp"

namespace i2c{
    void i2c::init(){
        
        
    }

    void i2c::write(uint8_t * dataBuffer, uint32_t size){

    }

    void i2c::read(uint8_t * dataBuffer, uint32_t size){

    }

    extern "C" void i2c1::I2C1_EV_IRQHandler(){

    } 

    extern "C" void i2c1::I2C1_ER_IRQHandler(){

    } 
}
