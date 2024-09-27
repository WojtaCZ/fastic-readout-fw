#include "si5340.hpp"
#include "si5340_config.h"

#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

bool lol, los;

namespace si5340 {
    using namespace stmcpp::units;
    // Loss of signal, loss of lock and interrupt status pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 0> los_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 1> lol_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 2> int_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    // Reset and output enable pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 3> oe_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 4> rst_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull, stmcpp::gpio::pull::pullUp);
    // Configure the i2c pins and interface
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 6> scl (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 7> sda (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c1> i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    //setup=scldel hold=(sdadel) high low

    stmcpp::i2c::address address (0x77);

    void init () {
        // Disable the outputs and enable the IC
        oe_n.set();
        rst_n.set();

        // Wait for the chip to wake up
        stmcpp::clock::systick::waitBlocking(300_ms);

        // Enable I2C
        i2c.enable();
        // Select the configuration page 0
        i2c.writeRegister(0x01, 0x00, address);

        // Check that we have the D revision
        if (i2c.readRegister(0x05, address) != 0x03) {
            return;
        }

        sendMagicPreamble();

        uint8_t pageOld = 0xFF, page, add;
        for(int i = 0; i < SI5340_REVD_REG_CONFIG_NUM_REGS; i++){
            si5340_revd_register_t reg = si5340_revd_registers[i];
            page = reg.address >> 8;

            // If the page has changed, select it
            if(page != pageOld){
                i2c.writeRegister(0x01, page, address);
                pageOld = page;
            }

            add = reg.address & 0xFF;

            i2c.writeRegister(add, reg.value, address);
        }   

        sendMagicPostamble();

        stmcpp::clock::systick::waitBlocking(100_ms);

        
        oe_n.clear();

        i2c.writeRegister(0x01, 0x00, address);
        i2c.writeRegister(0x1C, 0x01, address);

        stmcpp::clock::systick::waitBlocking(300_ms);

        // Enable interrupt on the INT pin of the clock generator
        int_n.enableInterrupt(stmcpp::gpio::interrupt::edge::falling);
        NVIC_EnableIRQ(EXTI2_IRQn);

        // Select page 0
        i2c.writeRegister(0x01, 0x00, address);
        std::uint8_t status = i2c.readRegister(0x0C, address);

        if (status & 0x01) {
            errorHandler.hardThrow(error::calibrating);
        } else if (status & 0x02) {
            errorHandler.hardThrow(error::losxtal);
        } else if (status & 0x04) {
            errorHandler.hardThrow(error::losref);
        } else if (status & 0x08) {
            errorHandler.hardThrow(error::lol);
        }

        // If no error is active, clean the flags that might have occured during setup
        i2c.writeRegister(0x11, 0x00, address);
        i2c.writeRegister(0x12, 0x00, address);
    }

    void sendMagicPreamble() {
        // Select the configuration page B and write a magic config preamble
        i2c.writeRegister(0x01, 0x0B, address);
        i2c.writeRegister(0x24, 0xC0, address);
        i2c.writeRegister(0x25, 0x00, address);

        // Some more magic numbers from the datasheet
        i2c.writeRegister(0x01, 0x05, address);
        i2c.writeRegister(0x02, 0x01, address);
        i2c.writeRegister(0x05, 0x03, address);

        i2c.writeRegister(0x01, 0x09, address);
        i2c.writeRegister(0x57, 0x17, address);

        i2c.writeRegister(0x01, 0x0B, address);
        i2c.writeRegister(0x4E, 0x1A, address);

        // Wait for the config to be applied
        stmcpp::clock::systick::waitBlocking(300_ms);
    }

    void sendMagicPostamble() {
        // Select the configuration page B and write a magic config preamble
        i2c.writeRegister(0x01, 0x0B, address);
        i2c.writeRegister(0x24, 0xC0, address);
        i2c.writeRegister(0x25, 0x00, address);

        // Some more magic numbers from the datasheet
        i2c.writeRegister(0x01, 0x05, address);
        i2c.writeRegister(0x02, 0x01, address);
        i2c.writeRegister(0x05, 0x03, address);

        i2c.writeRegister(0x01, 0x09, address);
        i2c.writeRegister(0x57, 0x17, address);

        i2c.writeRegister(0x01, 0x0B, address);
        i2c.writeRegister(0x4E, 0x1A, address);

        // Wait for the config to be applied
        stmcpp::clock::systick::waitBlocking(300_ms);
    }

    extern "C" void EXTI2_IRQHandler(void) {
        __ASM volatile("bkpt");
        if (int_n.getInterruptFlag()) {

            if (!lol_n.read()) {
                errorHandler.hardThrow(error::lol);
            } else if (!los_n.read()){
                errorHandler.hardThrow(error::losxtal);
            } else {
                errorHandler.hardThrow(error::other); 
            }

            int_n.clearInterruptFlag();
            NVIC_ClearPendingIRQ(EXTI2_IRQn);
        }
    }
}