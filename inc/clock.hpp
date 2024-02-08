#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>

namespace clock{
    //Initialize the clock -> SYSCLK = 480MHz, timer clock = 240MHz
    [[nodiscard]] std::uint8_t init();
}

#endif
