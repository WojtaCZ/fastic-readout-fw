#include "si5340.hpp"
#include "Si5340_config.h"


#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"
#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/units.hpp>

std::uint8_t opreg;

namespace si5340 {
    using namespace stmcpp::units;
    // Loss of signal, loss of lock and interrupt status pins
    /*stmcpp::gpio::pin<stmcpp::gpio::port::portd, 0> los_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 1> lol_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 2> int_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);*/
    // Reset and output enable pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 3> oe_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 4> rst_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull, stmcpp::gpio::pull::pullUp);
    // Configure the i2c pins and interface
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 6> scl (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 7> sda (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c1> i2c (0x0B, 4, 2, 0x0F, 0x13);
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

        stmcpp::clock::systick::waitBlocking(300_ms);

        
        oe_n.clear();

        i2c.writeRegister(0x01, 0x00, address);
        i2c.writeRegister(0x1C, 0x01, address);

        
        stmcpp::clock::systick::waitBlocking(300_ms);
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
}