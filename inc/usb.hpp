#ifndef USB_H
#define USB_H

#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/units.hpp>


namespace usb {
    enum class error {
        ulpi_timeout,
        ahb_idle_timeout,
        core_reset_timeout,
        other
    };

    static stmcpp::error::handler<error, "usb"> errorHandler;

    namespace interface {
      void init();
      void reset();
      std::uint8_t readRegister(std::uint8_t address);
      void writeRegister(std::uint8_t address, std::uint8_t data);
    }

    namespace core {
        void init();
        void softReset();
    }
}

#endif