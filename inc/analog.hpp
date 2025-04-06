#ifndef ANALOG_H
#define ANALOG_H

#include <stmcpp/error.hpp>
#include <cstdint>

namespace analog {
    void init();
    void calibrateVref();
    void calibrateVoltageMultiplier();
    double getVoltageMultiplier();
    float getFastICVoltage(uint8_t id);
    float getVbatVoltage();
    float getTemperature();
} 


#endif