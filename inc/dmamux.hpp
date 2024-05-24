#ifndef DMAMUX_H
#define DMAMUX_H

#include <cstdint>
#include <cstddef>
#include "register.hpp"
#include "stm32h753xx.h"

namespace dmamux{
    enum class peripheral {
        dmamux1_req_gen0,
        dmamux1_req_gen1,
        dmamux1_req_gen2,
        dmamux1_req_gen3,
        dmamux1_req_gen4,
        dmamux1_req_gen5,
        dmamux1_req_gen6,
        dmamux1_req_gen7,
        adc1_dma,
        adc2_dma,
        tim1_ch1,
        tim1_ch2,
        tim1_ch3,
        tim1_ch4,
        tim1_up,
        tim1_trig,
        tim1_com,
        tim2_ch1,
        tim2_ch2,
        tim2_ch3,
        tim2_ch4,
        tim2_up,
        tim3_ch1,
        tim3_ch2,
        tim3_ch3,
        tim3_ch4,
        tim3_up,
        tim3_trig,
        tim4_ch1,
        tim4_ch2,
        tim4_ch3,
        tim4_up,
        i2c1_rx_dma,
        i2c1_tx_dma,
        i2c2_rx_dma,
        i2c2_tx_dma,
        usart2_tx_dma,
        usart3_rx_dma,
        usart3_tx_dma,
        tim8_ch1,
        tim8_ch2,
        tim8_ch3,
        tim8_ch4,
        tim8_up,
        tim8_trig,
        tim8_com,
        //54
        tim5_ch1,
        tim5_ch2,
        tim5_ch3,
        tim5_ch4,
        tim5_up,
        tim5_trig,
        spi3_rx_dma,
        spi3_tx_dma,
        uart4_rx_dma,
        uart4_tx_dma,
        uart5_rx_dma,
        uart5_tx_dma,
        dac_ch1_dma,
        dac_ch2_dma,
        tim6_up,
        tim7_up,
        usart6_rx_dma,
        usart6_tx_dma,
        i2c3_rx_dma,
        i2c3_tx_dma,
        dcmi_dma,
        cryp_in_dma,
        cryp_out_dma,
        hash_in_dma,
        uart7_rx_dma,


    };

}



#endif
