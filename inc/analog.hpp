#ifndef ANALOG_H
#define ANALOG_H

#include <stmcpp/error.hpp>

namespace analog {



    void init();
    void calibrateVref();
    void calibrateVoltageMultiplier();
    double getVoltageMultiplier();
    double getFastIC2Voltage();
    double getFastIC1Voltage();
    double getVbatVoltage();
    double getTemperature();
} 


#endif