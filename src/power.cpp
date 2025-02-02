#include "power.hpp"
#include <stmcpp/gpio.hpp>

namespace power { 

    // 1V2 power control pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 1> pg_1V2D(stmcpp::gpio::mode::input);
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 2> pg_1V2T(stmcpp::gpio::mode::input);
    stmcpp::gpio::pin<stmcpp::gpio::port::portg, 2> pg_1V2A(stmcpp::gpio::mode::input);
    
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 5> en_1V2(stmcpp::gpio::mode::output);

    // 1V8 power control pin
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 9> pg_1V8D(stmcpp::gpio::mode::input);

    // 3V3A power control pin
    stmcpp::gpio::pin<stmcpp::gpio::port::portg, 10> pg_3V3A(stmcpp::gpio::mode::input);


    bool isPowerGood(ldo regulator) {
        switch(regulator) {
            case ldo::D1V2:
                return pg_1V2D.read();
                break;
            case ldo::T1V2:
                return pg_1V2T.read();
                break;
            case ldo::A1V2:
                return pg_1V2A.read();
                break;
            case ldo::D1V8:
                return pg_1V8D.read();
                break;
            case ldo::A3V3:
                return pg_3V3A.read();
                break;

            default:
                return false;
                break;
        }
    }

    void enable1V2() { 
        en_1V2.set();
    }

    void disable1V2() {
        en_1V2.clear();
    }
} 
