#ifndef ANALOG_H
#define ANALOG_H

#include <stmcpp/error.hpp>
#include "fastic.hpp"

namespace analog {
    void init();
    void calibrateVref();
    void calibrateVoltageMultiplier();
    double getVoltageMultiplier();
    float getFastICVoltage(fastic::identifier id);
    float getVbatVoltage();
    float getTemperature();
} 


#endif