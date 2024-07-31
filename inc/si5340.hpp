#ifndef SI5340_H
#define SI5340_H

#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/units.hpp>

namespace si5340 {
    enum class error {
        calibrating,
        losxtal,
        losref,
        lol,
        other
    };

    static stmcpp::error::handler<error, "si5340"> errorHandler;

    void init();
    void handleError();
    void sendMagicPreamble();
    void sendMagicPostamble();
}

#endif