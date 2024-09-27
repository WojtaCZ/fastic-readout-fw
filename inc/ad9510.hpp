#ifndef AD9510_H
#define AD9510_H

#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"
#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/error.hpp>

#include <cstdint>

namespace ad9510 {
    enum class error {
        dividers,
        outputs,
        phases,
        powerdown
    };

    static stmcpp::error::handler<error, "ad9510"> errorHandler;

    enum class channel {
        out5 = 0x35,
        out6 = 0x39
    };

    enum class rampCurrent {
        u200  = 0b000,
        u400  = 0b001,
        u600  = 0b010, 
        u800  = 0b011,
        u1000 = 0b100,
        u1200 = 0b101,
        u1400 = 0b110,
        u1600 = 0b111
    };

    enum class rampCaps {
        c1 = 0b111,
        c2 = 0b110,
        c3 = 0b010, 
        c4 = 0b000
    };

    void init();
    std::uint8_t transfer(bool read, std::uint8_t address, std::uint8_t data);
    std::uint8_t read(std::uint8_t address);
    std::uint8_t write(std::uint8_t address, std::uint8_t data);
    void setChannelDelayCoarse(channel ch, rampCaps caps, rampCurrent curr);
    void setChannelDelayFine(channel ch, uint8_t fineAdjust);
    void enableChannelDelay(channel ch);

}

#endif