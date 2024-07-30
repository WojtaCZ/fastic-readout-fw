#include "ad9510.hpp"

#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/units.hpp>

std::uint8_t testByte;

namespace ad9510 {
    using namespace stmcpp::units;
    // Control interface pins, the communication needs to be bitbanged due to not having enough SPIs :( 
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 13> cs  (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 8>  clk (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 7>  mosi (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 12> miso (stmcpp::gpio::mode::input);
    
    void init() {
        testByte = read(0x3C);
        testByte = read(0x3D);
        testByte = read(0x52);
    }

    std::uint8_t write(std::uint8_t address, std::uint8_t data) {
        // Set pins to default state
        cs.set();
        clk.clear();
        mosi.clear();

        // The AD9510 instruction word is
        // [15]     = R/Wn
        // [14:13]  = Number of bytes to transfer
        // [6:0]    = Address

        std::uint16_t instructionWord_ = (1 << 15) |    // Read operation
                                         (0 << 13) |    // One byte
                                         (address & 0x7F);
        
        std::uint32_t outputFrame_ = (instructionWord_ << 16) | (data) << 8;

        std::uint32_t inputFrame_ = 0;

        // Assert the chip select and wait a little
        int i = 0;
        for (i = 0; i < 40; i++) __ASM volatile("nop");
        cs.clear();
        __ASM volatile("nop");
        __ASM volatile("nop");
        __ASM volatile("nop");
        __ASM volatile("nop");
        __ASM volatile("nop");
        __ASM volatile("nop");
        __ASM volatile("nop");
        
        
        // Shift out the data
        for (int bit = 0; bit < 24; bit++){

            mosi.write(outputFrame_ & 0x80000000);
            outputFrame_ <<= 1;
            __ASM volatile("nop");

            clk.set();
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");

            inputFrame_ |= (miso.read() & 0x00000001);
            inputFrame_ <<= 1;
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");

            clk.clear();
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
            __ASM volatile("nop");
        }

        clk.clear();
        mosi.clear();
        cs.set();

        return ((inputFrame_ >> 1) & 0xFF);
    }

    std::uint8_t read(std::uint8_t address) {
        return write(address, 0x00);
    }
}