#ifndef USB_H
#define USB_H

#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/units.hpp>

extern "C" uint32_t USB_ULPI_Read(uint32_t Addr);
extern "C" uint32_t USB_ULPI_Write(uint32_t Addr, uint32_t Data);

namespace usb {
    enum class error {
        other
    };

    static stmcpp::error::handler<error, "usb"> errorHandler;

    void init();
    void coreInit();
    void rst();
}

#endif