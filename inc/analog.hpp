#ifndef ANALOG_H
#define ANALOG_H

#include <stmcpp/error.hpp>

namespace analog {



    void init();
    void calibrateVref();
    void calibrateVoltageMultiplier();
    double getVoltageMultiplier();
    float getFastIC2Voltage();
    float getFastIC1Voltage();
    float getVbatVoltage();
    float getTemperature();
} 


#endif