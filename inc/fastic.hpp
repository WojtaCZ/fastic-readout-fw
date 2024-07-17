#ifndef FASTIC_H
#define FASTIC_H

#include<cstdint>

// 0x00 - frontend setup
enum class frontendpol : std::uint8_t {
    positive = 0b0,
    negative = 0b1
};

enum class frontendmode : std::uint8_t {
    singleended     = 0b00,
    differential    = 0b01,
    sum             = 0b10,
    // 0b11 = single ended again, why?
};

// 0x01 Channel enable 
enum class channel : std::uint8_t {
    channel0 = 0b00000001,
    channel1 = 0b00000010,
    channel2 = 0b00000100,
    channel3 = 0b00001000,
    channel4 = 0b00010000,
    channel5 = 0b00100000,
    channel6 = 0b01000000,
    channel7 = 0b10000000
}

// 0x02 Channel injection enable

// 0x03 Global LSB current adjustment
// 0x04 NMOS and PMOS DAC bias current

// 0x05 superslow and superfast bias

#endif