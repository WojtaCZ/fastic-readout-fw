#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>

#include "register.hpp"
#include "stm32h753xx.h"

namespace clock{
    //Initialize the clock -> SYSCLK = 480MHz, timer clock = 240MHz
    [[nodiscard]] std::uint8_t init();

    namespace pll{
        enum class peripheral : uint32_t {
            pll1 = offsetof(RCC_TypeDef, PLL1DIVR),
            pll2 = offsetof(RCC_TypeDef, PLL2DIVR),
            pll3 = offsetof(RCC_TypeDef, PLL3DIVR)
        };

        template<peripheral peripheral_, uint8_t mdivider_, uint8_t multiplier_, uint16_t multiplierFractions_, uint8_t pdivider_,uint8_t qdivider_, uint8_t rdivider_>
    }
}

#endif
