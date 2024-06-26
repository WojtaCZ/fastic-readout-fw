#ifndef I2C_H
#define I2C_H

#include <cstdint>
#include "stm32h753xx.h"

namespace i2c{
    enum class peripheral : uint32_t {
        i2c1 = I2C1_BASE, ///< I2C1 peripheral selected
        i2c2 = I2C2_BASE, ///< I2C2 peripheral selected
        i2c3 = I2C3_BASE, ///< I2C3 peripheral selected
        i2c4 = I2C4_BASE  ///< I2C4 peripheral selected
    };

    enum class interrupt : uint32_t {
        txinterrupt = (0b1 << I2C_ISR_TXIS_Pos),
        rxinterrupt = (0b1 << I2C_ISR_RXNE_Pos),
        addrmatched = (0b1 << I2C_ISR_ADDR_Pos),  ///< Address matched
        nack        = (0b1 << I2C_ISR_NACKF_Pos), ///< Not acknowledge
        stop        = (0b1 << I2C_ISR_STOPF_Pos), ///< STOP detection
        // These interrupts are enabled by the TCIE bit
        txcomplete  = (0b1 << I2C_ISR_TC_Pos),    ///< Transfer complete
        txreload    = (0b1 << I2C_ISR_TCR_Pos),   ///< Transfer complete reload
        // These interrupts are enabled by the ERRIE bit
        buserror    = (0b1 << I2C_ISR_BERR_Pos),  ///< Bus error
        arbitration = (0b1 << I2C_ISR_ARLO_Pos),  ///< Arbitration lost
        overrun     = (0b1 << I2C_ISR_OVR_Pos),   ///< Overrun/Underrun 
    };

    enum class status : uint32_t {
        txempty     = (0b1 << I2C_ISR_TXE_Pos),  ///< TX buffer empty
        busy        = (0b1 << I2C_ISR_BUSY_Pos)  ///< Communication on the bus is in progress
    };

    template<peripheral Peripheral>
    class i2c{
        public:
            constexpr i2c(uint8_t prescaler, uint8_t setuptime, uint8_t holdtime, uint8_t highperiod, uint8_t lowperiod){
                
            }

    };
}

#endif


class spi{
    
}