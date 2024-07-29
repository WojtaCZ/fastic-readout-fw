#ifndef SI5340_H
#define SI5340_H

#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>

namespace si5340 {
    void init ();
}

#endif