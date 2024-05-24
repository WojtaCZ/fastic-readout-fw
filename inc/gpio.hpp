#ifndef GPIO_H
#define GPIO_H

#include <cstdint>
#include <cstddef>
#include "stm32h753xx.h"

#include "register.hpp"

namespace gpio{
    enum class port : uint32_t {
        porta = GPIOA_BASE,
        portb = GPIOB_BASE,
        portc = GPIOC_BASE,
        portd = GPIOD_BASE,
        
        #ifdef GPIOE_BASE
        porte = GPIOE_BASE,
        #endif

        #ifdef GPIOF_BASE
        portf = GPIOF_BASE,
        #endif

        #ifdef GPIOG_BASE
        portg = GPIOG_BASE,
        #endif

        #ifdef GPIOH_BASE
        porth = GPIOH_BASE,
        #endif

        #ifdef GPIOI_BASE
        porti = GPIOI_BASE,
        #endif

        #ifdef GPIOJ_BASE
        portj = GPIOJ_BASE,
        #endif

        #ifdef GPIOK_BASE
        portk = GPIOK_BASE,
        #endif

        #ifdef GPIOZ_BASE
        portz = GPIOZ_BASE
        #endif
    };

    enum class mode{
        input     = 0b00000000,
        pushpull  = 0b00000001,
        opendrain = 0b00000101,
        af0       = 0b00000010,
        af1       = 0b00010010,
        af2       = 0b00100010,
        af3       = 0b00110010,
        af4       = 0b01000010,
        af5       = 0b01010010,
        af6       = 0b01100010,
        af7       = 0b01110010,
        af8       = 0b10000010,
        af9       = 0b10010010,
        af10      = 0b10100010,
        af11      = 0b10110010,
        af12      = 0b11000010,
        af13      = 0b11010010,
        af14      = 0b11100010,
        af15      = 0b11110010,
        analog    = 0b00000011
    };

    enum class speed{
        low       = 0b00,
        medium    = 0b01,
        high      = 0b10,
        veryhigh  = 0b11
    };

    enum class pull{
        nopull    = 0b00,
        pullup    = 0b01,
        pulldown  = 0b10
    };

    /// @brief GPIO pin class
    /// @param port GPIO_TypeDef of the port
    /// @param pin Pin number
    /// @param m GPIO Mode
    /// @param s GPIO Speed
    /// @param p GPIO Pullup/Pulldown
    template<port port_, uint8_t pin_, mode mode_, speed speed_, pull pull_>
    class gpio{
        public:
            gpio(){
                static_assert(pin_ < 15, "The pin number cannot be greater than 15!");
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, MODER)>(0b11 << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, MODER)>((static_cast<uint8_t>(mode_) & 0b00000011) << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OTYPER)>(((static_cast<uint8_t>(mode_) & 0b00000100) >> 2) << pin_);
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, AFR[static_cast<uint8_t>(pin_ / 8)])>(((static_cast<uint8_t>(mode_) & 0b11110000) >> 4) << ((pin_ % 8) * 4));
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OSPEEDR)>(0b11 << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OSPEEDR)>(static_cast<uint8_t>(speed_) << (pin_ * 2));
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, PUPDR)>(0b11 << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, PUPDR)>(static_cast<uint8_t>(pull_) << (pin_ * 2));
            };

            void set(){
                reg::write<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, BSRR)>(1 << pin_);
            }

            void clear(){
                reg::write<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, BSRR)>(1 << (pin_ + 16));
            }

            void write(bool state){
                state ? set() : clear();
            }

            void toggle(){
                reg::toggle<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, ODR)>(1 << pin_);
            }

            bool read(){
                return static_cast<bool>(reg::read<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, IDR)>(1 << pin_));
            }

            void setMode(mode m){
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, MODER)>(0b0011 << (pin_ * 2));
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OTYPER)>(0b0001 << pin_);
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, AFR[static_cast<uint8_t>(pin_ / 8)])>(0b1111 << ((pin_ % 8) * 4));

                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, MODER)>((static_cast<uint8_t>(m) & 0b00000011) << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OTYPER)>(((static_cast<uint8_t>(m) & 0b00000100) >> 2) << pin_);
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, AFR[static_cast<uint8_t>(pin_ / 8)])>(((static_cast<uint8_t>(m) & 0b11110000) >> 4) << ((pin_ % 8) * 4));
            }

            void setSpeed(speed s){
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OSPEEDR)>(0b11 << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, OSPEEDR)>(static_cast<uint8_t>(s) << (pin_ * 2));
            }

            void setPull(pull p){
                reg::clear<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, PUPDR)>(0b11 << (pin_ * 2));
                reg::set<static_cast<uint32_t>(port_) + offsetof(GPIO_TypeDef, PUPDR)>(static_cast<uint8_t>(p) << (pin_ * 2));
            }
    };
    
}

#endif
