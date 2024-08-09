#ifndef STMCPP_CONFIG_H
#define STMCPP_CONFIG_H

#include <stmcpp/units.hpp>

#define STM32H7

namespace stmcpp::config{
    using namespace stmcpp::units;
    //Allows bigger range for voltage units
    //#define STMCPP_UNITS_VOLTAGE_HIGHRANGE
    //Allows bigger range for voltage units
    //#define STMCPP_UNITS_CURRENT_HIGHRANGE
    //Allows bigger range for duration (~35minutes @ 1us resolution VS ~213days @ 1ps resolution)
    //#define STMCPP_UNITS_DURATION_HIGHRANGE

    //Clock configuration of the MCU
    static constexpr auto hse = 48_MHz;
    static constexpr auto hsi = 64_MHz;
    static constexpr auto csi = 4_MHz;
    static constexpr auto lse = 32768_Hz;
    static constexpr auto lsi = 32_kHz;

    //System clock
    static constexpr auto sysclock = 480_MHz;



}

namespace stmcpp::clock::pll {
     #ifdef STM32H7
        //Ranges for divisor values
        constexpr std::pair<unsigned int, unsigned int> range_div_m = {1u,  63u};
        constexpr std::pair<unsigned int, unsigned int> range_div_n = {4u, 512u};
        constexpr std::pair<unsigned int, unsigned int> range_div_p = {2u, 128u};
        constexpr std::pair<unsigned int, unsigned int> range_div_q = {1u, 128u};
        constexpr std::pair<unsigned int, unsigned int> range_div_r = {1u, 128u};
    #endif
}

#endif