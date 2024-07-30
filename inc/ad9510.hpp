#ifndef AD9510_H
#define AD9510_H

#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <cstdint>

namespace ad9510 {
    void init();
    std::uint8_t read(std::uint8_t address);
    std::uint8_t write(std::uint8_t address, std::uint8_t data);
}

#endif