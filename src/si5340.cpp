#include "si5340.hpp"

namespace si5340 {
    // Loss of signal, loss of lock and interrupt status pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 0> los_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 1> lol_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 2> int_n (stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullUp);
    // Reset and output enable pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 3> oe_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::pullUp);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 4> rst_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::pullUp);
    // Configure the i2c pins and interface
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 6> scl (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 7> sda (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c1> i2c (0x0B, 4, 2, 0x0F, 0x13);
    stmcpp::i2c::address address (0x75);

    void init () {
        i2c.enable();
	    std::uint8_t data = i2c.read8bitAddress(0x0B, address);
    }
}