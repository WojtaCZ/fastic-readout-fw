#ifndef SPI_H
#define SPI_H

#include "stm32h753xx.h"
#include <cstdint>

namespace spi{
    template<SPI_TypeDef * periph>
    class spi {
        public:
            spi();
        private:
            uint32_t * rxBuff;
    };
}

#endif
