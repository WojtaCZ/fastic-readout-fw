#ifndef I2C_H
#define I2C_H

#include <cstdint>
#include "stm32h753xx.h"



namespace i2c{
    class i2c {
        protected:
            I2C_TypeDef * peripheral_;
            uint16_t address_; 
            i2c(){};

            void write(uint8_t * dataBuffer, uint32_t size);
            void read(uint8_t * dataBuffer, uint32_t size);
            void init();
    };

    class i2c1 : public i2c{
        private:
            void I2C1_EV_IRQHandler();
            void I2C1_ER_IRQHandler();

            void (*errorHandler)(void);
            void (*eventHandler)(void);
        public:
            i2c1(uint16_t address){
                this->peripheral_ = I2C1;
                this->address_ = address;
            };
            
    };
}

#endif
