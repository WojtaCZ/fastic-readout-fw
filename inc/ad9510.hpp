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

    void init();
    std::uint8_t transfer(bool read, std::uint8_t address, std::uint8_t data);
    std::uint8_t read(std::uint8_t address);
    std::uint8_t write(std::uint8_t address, std::uint8_t data);
}

#endif