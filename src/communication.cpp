/*

set fastic [1/2/both] register [add] [val] [force]
get fastic [1/2/both] register [add] "get fastic (?:1|2|both) register 0x(?:[a-f0-9]){2}"



get bias voltage
set bias voltage [val]
get bias current

get readout info
get readout config

get userboard info
get userboard config
set userboard config

*/

#include "communication.hpp"
#include "power.hpp"
#include "analog.hpp"


#include <tinyusb/src/device/usbd.h>
#include <tinyusb/src/class/cdc/cdc_device.h>
#include <tinyusb/src/class/vendor/vendor_device.h>

namespace communication {

    struct statusMessage {
        uint32_t header;

        uint8_t pg1V2D : 1; 
        uint8_t pg1V2T : 1;
        uint8_t pg1V2A : 1;
        uint8_t pg1V8D : 1;
        uint8_t pg3V3A : 1;
        uint8_t en1V2 : 1;
        uint8_t enHV : 1;
        uint8_t blank : 1;
        

        double mcuTemperature;
        double mcuVoltage;
        double fastIC1Voltage;
        double fastIC2Voltage;
        double hvVoltage;
        double hvCurrent;
    };

    void process(){
        /*if(tud_vendor_n_available(0)){
            uint32_t count = tud_vendor_n_read(buf, sizeof(buf));
            if(count){
                (void) count;
                
                int value = atoi(buf);
                //printf("Received: %d\n\r", value);
                
                hv::setVoltage(value);
            }
        }*/
    }

    void sendStatus(){
        if(tud_vendor_mounted()){
            statusMessage status = {
                .header = 0xABCD1234, // Example header value
                .pg1V2D = power::isPowerGood(power::ldo::D1V2),
                .pg1V2T = power::isPowerGood(power::ldo::T1V2),
                .pg1V2A = power::isPowerGood(power::ldo::A1V2),
                .pg1V8D = power::isPowerGood(power::ldo::D1V8),
                .pg3V3A = power::isPowerGood(power::ldo::A3V3),
                .en1V2 = power::is1V2Enabled(),
                .enHV = hv::isEnabled(),
                .blank = 0,
                .mcuTemperature = analog::getTemperature(), // Example temperature value
                .mcuVoltage = analog::getVbatVoltage(), // Example voltage value
                .fastIC1Voltage = analog::getFastIC1Voltage(), // Example voltage value
                .fastIC2Voltage = analog::getFastIC2Voltage(), // Example voltage value
                .hvVoltage = hv::getVoltage(), // Example high voltage value
                .hvCurrent = hv::getCurrent() // Example current value
            };

            uint8_t buffer[64];
            memcpy(buffer, &status, sizeof(statusMessage));

            tud_vendor_n_write_flush(0);
            tud_vendor_n_write(0, &buffer, 64);
        }
    }

}