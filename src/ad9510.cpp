#include "ad9510.hpp"

namespace ad9510 {
    using namespace stmcpp::units;

    uint8_t reg = 0;

    // Control interface pins, the communication needs to be bitbanged due to not having enough SPIs :( 
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 13> cs  (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 8>  clk (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 7>  mosi (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 12> miso (stmcpp::gpio::mode::input);

    static constexpr std::uint8_t OUT4_DIV_ADD = 0x50;
    static constexpr std::uint8_t OUT5_DIV_ADD = 0x52;
    static constexpr std::uint8_t OUT6_DIV_ADD = 0x54;
    static constexpr std::uint8_t OUT7_DIV_ADD = 0x56;

    static constexpr std::uint8_t OUT4_PHA_ADD = 0x51;
    static constexpr std::uint8_t OUT5_PHA_ADD = 0x53;
    static constexpr std::uint8_t OUT6_PHA_ADD = 0x55;
    static constexpr std::uint8_t OUT7_PHA_ADD = 0x57;

    static constexpr std::uint8_t OUT0_LVPECL_ADD = 0x3C;
    static constexpr std::uint8_t OUT1_LVPECL_ADD = 0x3D;
    static constexpr std::uint8_t OUT2_LVPECL_ADD = 0x3E;
    static constexpr std::uint8_t OUT3_LVPECL_ADD = 0x3F;

    static constexpr std::uint8_t OUT4_LVDS_ADD = 0x40;
    static constexpr std::uint8_t OUT5_LVDS_ADD = 0x41;
    static constexpr std::uint8_t OUT6_LVDS_ADD = 0x42;
    static constexpr std::uint8_t OUT7_LVDS_ADD = 0x43;

    static constexpr std::uint8_t UPD_REG_ADD = 0x5A;
    static constexpr std::uint8_t SYNC_REG_ADD = 0x58;
    static constexpr std::uint8_t PLL_REG_ADD = 0x0A;
    static constexpr std::uint8_t SRC_REG_ADD = 0x45;

    static constexpr std::uint8_t OUT4_DIV_VAL = 0x11;   // Divide by 4, 50% duty cycle //11
    static constexpr std::uint8_t OUT5_DIV_VAL = 0x11;   // Divide by 4, 50% duty cycle //11
    static constexpr std::uint8_t OUT6_DIV_VAL = 0x33;   // Divide by 8, 50% duty cycle
    static constexpr std::uint8_t OUT7_DIV_VAL = 0x33;   // Divide by 8, 50% duty cycle

    static constexpr std::uint8_t OUT4_PHA_VAL = 0x00;   // Phase shift by 90 deg /0x01
    static constexpr std::uint8_t OUT5_PHA_VAL = 0x00;   // Phase shift by 90 deg
    static constexpr std::uint8_t OUT6_PHA_VAL = 0x00;   // Phase shift by 0 deg
    static constexpr std::uint8_t OUT7_PHA_VAL = 0x00;   // Phase shift by 0 deg

    static constexpr std::uint8_t OUT0_LVPECL_VAL = 0x03;   // Complete power down
    static constexpr std::uint8_t OUT1_LVPECL_VAL = 0x03;   // Complete power down
    static constexpr std::uint8_t OUT2_LVPECL_VAL = 0x03;   // Complete power down
    static constexpr std::uint8_t OUT3_LVPECL_VAL = 0x03;   // Complete power down

    static constexpr std::uint8_t OUT4_LVDS_VAL = 0x02;  // LVDS, 3.5mA, 100R termination
    static constexpr std::uint8_t OUT5_LVDS_VAL = 0x02;  // LVDS, 3.5mA, 100R termination
    static constexpr std::uint8_t OUT6_LVDS_VAL = 0x02;  // LVDS, 3.5mA, 100R termination
    static constexpr std::uint8_t OUT7_LVDS_VAL = 0x02;  // LVDS, 3.5mA, 100R termination

    static constexpr std::uint8_t PLL_REG_VAL = 0x03;    // Power down the PLL section
    static constexpr std::uint8_t SRC_REG_VAL = 0x1D;    // Make CK1 the clock source, power down the others

void init() {

        // Power down the PLL
        write(PLL_REG_ADD, PLL_REG_VAL);
        // Make CK1 the clock source, power down the others
        write(SRC_REG_ADD, SRC_REG_VAL);
        // Power down the unused outputs
        write(OUT0_LVPECL_ADD, OUT0_LVPECL_VAL);
        write(OUT1_LVPECL_ADD, OUT1_LVPECL_VAL);
        write(OUT2_LVPECL_ADD, OUT2_LVPECL_VAL);
        write(OUT3_LVPECL_ADD, OUT3_LVPECL_VAL);

        // SPI clocks setup
        // Setup OUT4 and OUT5 divisors
        write(OUT4_DIV_ADD, OUT4_DIV_VAL);
        write(OUT5_DIV_ADD, OUT5_DIV_VAL);
        // Enable them in LVDS mode
        write(OUT4_LVDS_ADD, OUT4_LVDS_VAL);
        write(OUT5_LVDS_ADD, OUT5_LVDS_VAL);
        // Adjust the phase of the SPI clocks
        write(OUT4_PHA_ADD, OUT4_PHA_VAL);
        write(OUT5_PHA_ADD, OUT5_PHA_VAL);

        // Setup OUT6 and OUT7 to divide by 8 - FASTIC clocks
        write(OUT6_DIV_ADD, OUT6_DIV_VAL);
        write(OUT7_DIV_ADD, OUT7_DIV_VAL);
        // Enable them in LVDS mode
        write(OUT6_LVDS_ADD, OUT6_LVDS_VAL);
        write(OUT7_LVDS_ADD, OUT7_LVDS_VAL);
        // Adjust the phase of the Fastic clocks
        write(OUT6_PHA_ADD, OUT6_PHA_VAL);
        write(OUT7_PHA_ADD, OUT7_PHA_VAL);

        //write(OUT6_PHA_ADD, OUT6_PHA_VAL);
        //write(OUT7_PHA_ADD, OUT7_PHA_VAL);

        //Update the registers with the config
        write(UPD_REG_ADD, 0x01);

        //Sync the clocks
        write(SYNC_REG_ADD, 0x04);
        write(SYNC_REG_ADD, 0x00);

        // Check if the dividers are set up correctly
        if ((read(OUT4_DIV_ADD) != OUT4_DIV_VAL) || (read(OUT5_DIV_ADD) != OUT5_DIV_VAL) || (read(OUT6_DIV_ADD) != OUT6_DIV_VAL) || (read(OUT7_DIV_ADD) != OUT7_DIV_VAL)) {
            errorHandler.hardThrow(error::dividers);
        }

        // Check the modes of the outputs
        if ((read(OUT4_LVDS_ADD) != OUT4_LVDS_VAL) || (read(OUT5_LVDS_ADD) != OUT5_LVDS_VAL) || (read(OUT6_LVDS_ADD) != OUT6_LVDS_VAL) || (read(OUT7_LVDS_ADD) != OUT7_LVDS_VAL)) {
            errorHandler.hardThrow(error::outputs);
        }   

        // Check the phases of the registers
        if ((read(OUT4_PHA_ADD) != OUT4_PHA_VAL) || (read(OUT5_PHA_ADD) != OUT5_PHA_VAL) || (read(OUT6_PHA_ADD) != OUT6_PHA_VAL) || (read(OUT7_PHA_ADD) != OUT7_PHA_VAL)) {
            errorHandler.hardThrow(error::phases);
        }

        // Check if all not needed stuff has been powered down
        if ((read(PLL_REG_ADD) != PLL_REG_VAL) ||
            (read(SRC_REG_ADD) != SRC_REG_VAL) ||
            (read(OUT0_LVPECL_ADD) != OUT0_LVPECL_VAL) ||
            (read(OUT1_LVPECL_ADD) != OUT1_LVPECL_VAL) ||
            (read(OUT2_LVPECL_ADD) != OUT2_LVPECL_VAL) ||
            (read(OUT3_LVPECL_ADD) != OUT3_LVPECL_VAL)) {
                errorHandler.hardThrow(error::powerdown);
        }

        
    }

    std::uint8_t transfer(bool read, std::uint8_t address, std::uint8_t data) {
        // Set pins to default state
        cs.set();
        clk.clear();
        mosi.clear();

        // The AD9510 instruction word is
        // [15]     = R/Wn
        // [14:13]  = Number of bytes to transfer
        // [6:0]    = Address

        std::uint16_t instructionWord_ = ((read & 0x01) << 15) |    // Operation type
                                         (0 << 13) |                // One byte
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
        for (i = 0; i < 100; i++) __ASM volatile("nop");

        return ((inputFrame_ >> 1) & 0xFF);
    }

    std::uint8_t read(std::uint8_t address) {
        return transfer(true, address, 0x00);
    }

    std::uint8_t write(std::uint8_t address, std::uint8_t data) {
        return transfer(false, address, data);
    }

    void enableChannelDelay(channel ch){
        write(static_cast<uint8_t>(ch) - 1, 0x00);
        write(UPD_REG_ADD, 0x01);
    }

    void disableChannelDelay(channel ch){
        write(static_cast<uint8_t>(ch) - 1, 0x01);
        write(UPD_REG_ADD, 0x01);
    }

    void setChannelDelayCoarse(channel ch, rampCaps caps, rampCurrent curr){
        write(static_cast<uint8_t>(ch), static_cast<uint8_t>(curr) | (static_cast<uint8_t>(caps) << 3));

        //Update the config
        write(UPD_REG_ADD, 0x01);

        //Sync the clocks
      //  write(SYNC_REG_ADD, 0x04);
       // write(SYNC_REG_ADD, 0x00);
    }

    void setChannelDelayFine(channel ch, uint8_t fineAdjust){
        write(static_cast<uint8_t>(ch) + 1, (fineAdjust & 0b11111) << 1);
        //Update the config
        write(UPD_REG_ADD, 0x01);

        //Sync the clocks
       // write(SYNC_REG_ADD, 0x04);
       // write(SYNC_REG_ADD, 0x00);

    }
}