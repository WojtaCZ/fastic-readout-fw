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
#include "fastic.hpp"
#include "git.hpp"
#include "si5340.hpp"


#include <tinyusb/src/device/usbd.h>
#include <tinyusb/src/class/cdc/cdc_device.h>
#include <tinyusb/src/class/vendor/vendor_device.h>

namespace communication {

    const char* commands[] = {
        "readout status",
        "readout id",
        "hv enable",
        "hv current",
        "hv voltage",
        "fastic register",
        "fastic voltage",
        "fastic syncreset",
        "fastic calpulse",
        "fastic time",
        "fastic aurora",
        "userboard status",
        "userboard register",
        "userboard id",
        "userboard name",
        "userboard voltage",
        "userboard tomemory",
        "userboard frommemory"
    };

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

    // Buffer for print out of multiline messages
    char printBuffer[512];

    char cdcBuffer[512];

    void process(){
        // connected and there are data available
        if (tud_cdc_available()) {
            uint32_t count = tud_cdc_read(cdcBuffer, sizeof(cdcBuffer));

            // There is no command smaller than 4 characters
            if(count < 4){
                printf("Unknown command!\n\r");
                tud_cdc_read_flush();
                return;
            }
        
            // Parse the direction
            direction dir = parseDirection(cdcBuffer);

            if(dir == direction::UNKNOWN){
                printf("Unknown command!\n\r");
                tud_cdc_read_flush();
                return;
            }

            // Parse the command
            command cmd = parseTextCommand(cdcBuffer);

            if(cmd == command::UNKNOWN){
                printf("Unknown command!\n\r");
                tud_cdc_read_flush();
                return;
            }

            if(processTextCommand(cmd, dir, cdcBuffer + strlen(commands[static_cast<uint8_t>(cmd)]) + 5)){
                printf("Command processed successfully\n\r");
                tud_cdc_read_flush();
            } else {
                printf("Command processing failed\n\r");
                tud_cdc_read_flush();
            }


        }
            
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

    direction parseDirection(char * textCommand){
        if(textCommand[0] == 'g'){
            // Get command
            return direction::GET;
        } else if (textCommand[0] == 's'){
            // Set command
            return direction::SET;
        } else return direction::UNKNOWN;
    }

    command parseTextCommand(char * textCommand){
        // Parse the commands based on the first letters
        if (textCommand[4] == 'r'){
            // Readout
            switch (textCommand[strlen("readout") + 5]){
                case 's':
                    return command::READOUT_STATUS;
                case 'i':
                    return command::READOUT_ID;
                default:
                    return command::UNKNOWN;
            }
        } else if (textCommand[4] == 'h'){
            // HV
            switch (textCommand[strlen("hv") + 5]){
                case 'e':
                    return command::HV_ENABLE;
                case 'c':
                    return command::HV_CURRENT;
                case 'v':
                    return command::HV_VOLTAGE;
                default:
                    return command::UNKNOWN;
            }
        } else if (textCommand[4] == 'f'){
            // FastIC
            switch (textCommand[strlen("fastic") + 5]){
                case 'r':
                    return command::FASTIC_REGISTER;
                case 'v':
                    return command::FASTIC_VOLTAGE;
                case 's':
                    return command::FASTIC_SYNCRESET;
                case 'c':
                    return command::FASTIC_CALPULSE;
                case 't':
                    return command::FASTIC_TIME;
                case 'a':
                    return command::FASTIC_AURORA;
                default:
                    return command::UNKNOWN;
            }
        } else if (textCommand[4] == 'u'){
            // Userboard
            switch (textCommand[strlen("userboard") + 5]){
                case 's':
                    return command::USERBOARD_STATUS;
                case 'r':
                    return command::USERBOARD_REGISTER;
                case 'i':
                    return command::USERBOARD_ID;
                case 'n':
                    return command::USERBOARD_NAME;
                case 'v':
                    return command::USERBOARD_VOLTAGE;
                case 't':
                    return command::USERBOARD_TOMEMORY;
                case 'f':
                    return command::USERBOARD_FROMMEMORY;
                default:
                    return command::UNKNOWN;
            }

        } else return command::UNKNOWN;
        
    }

    bool processTextCommand(command &cmd, direction &dir, char * params){
        switch (cmd){
            case command::READOUT_STATUS:

                if (dir == direction::GET) {
                    sprintf(printBuffer, "Readout temperature: %.2f [C]\n\r1V2 domain: %s\n\r   Power good 1V2D: %s\n\r   Power good 1V2T: %s\n\r   Power good 1V2A: %s\n\rOther domains:\n\r   3V3 voltage: %.2f [V]\n\r   Power good 3V3A: %s\n\r   Power good 1V8D: %s\n\rClock generator status:\n\r   Output enabled: %s\n\r   Reset: %s\n\r   Loss of Lock: %s\n\r   Loss of Signal: %s\n\r",
                        analog::getTemperature(),
                        power::is1V2Enabled() ? "enabled" : "disabled",
                        power::isPowerGood(power::ldo::D1V2) ? "good" : "bad",
                        power::isPowerGood(power::ldo::T1V2) ? "good" : "bad",
                        power::isPowerGood(power::ldo::A1V2) ? "good" : "bad",
                        analog::getVbatVoltage(),
                        power::isPowerGood(power::ldo::A3V3) ? "good" : "bad",
                        power::isPowerGood(power::ldo::D1V8) ? "good" : "bad",
                        si5340::isOutputEnabled() ? "disabled" : "enabled",
                        si5340::isReset() ? "high" : "low",
                        si5340::getLOL() ? "locked" : "lock lost",
                        si5340::getLOS() ? "signal ok" : "signal lost");
                    
                    tud_cdc_write(printBuffer, strlen(printBuffer));
                    tud_cdc_write_flush();

                    return true;
                } else {
                    printf("This command does not support SET!\n\r");
                    return false;
                }
                break;
            case command::READOUT_ID:

                if (dir == direction::GET) {
                    
                    sprintf(printBuffer, "Readout UID: %08X%08X%08X\n\rReadout SW:   \n\rCommit: %s\n\r   Branch: %s\n\r   Build date and time: %s %s\n\r",
                        *(uint32_t *)(UID_BASE), *(uint32_t *)(UID_BASE + 4), *(uint32_t *)(UID_BASE + 8),
                        git::revision.c_str(),
                        git::branch.c_str(),
                        git::build_date.c_str(),
                        git::build_time.c_str()
                    );

                    tud_cdc_write(printBuffer, strlen(printBuffer));
                    tud_cdc_write_flush();
                    return true;
                   // printf("Readout SW:\n\r   Commit: %s\r\n   Branch: %s\r\n   Build date and time: %s %s\r\n", git::revision, git::branch, git::build_date, git::build_time);
                } else {
                    printf("This command does not support SET!\n\r");
                    return false;
                }

                break;
            case command::HV_ENABLE:

                if (dir == direction::GET) {
                    printf("HV supply is %s\n\r", hv::isEnabled() ? "enabled" : "disabled");
                    return true;
                } else {
                    if(params[0] == 't'){
                        hv::enable();
                        printf("HV supply enabled\n\r");
                        return true;
                    } else if (params[0] == 'f'){
                        hv::disable();
                        printf("HV supply disabled\n\r");
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }
                    return false;
                }

                break;
            case command::HV_CURRENT:

                if (dir == direction::GET) {
                    printf("HV current: %.2f [uA]\n\r", hv::getCurrent());
                    return true;
                } else {
                    printf("This command does not support SET!\n\r");
                    return false;
                }

                break;
            case command::HV_VOLTAGE:

                if (dir == direction::GET) {
                    printf("HV voltage: %.2f [V]\n\r", hv::getVoltage());
                    return true;
                } else {
                    float voltage = atof(params);

                    if(voltage < 0 || voltage > 80){
                        printf("Voltage out of range!\n\r");
                        return false;
                    }
                    
                    hv::setVoltage(voltage);

                    printf("Set HV voltage to: %.2f [V]\n\r", voltage);
                    return true;
                }

                break;
            case command::FASTIC_REGISTER:

                if (dir == direction::GET) {
                    // Process get FastIC register command
                } else {
                    // Process set FastIC register command
                } 

                break;
            case command::FASTIC_VOLTAGE:

                if (dir == direction::GET) {

                    if(params[0] == '1'){
                        printf("FastIC 1 VMON voltage: %.3f [V]\n\r", analog::getFastIC1Voltage());
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 VMON voltage: %.3f [V]\n\r", analog::getFastIC2Voltage());
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }

                } else {

                    printf("This command does not support SET!\n\r");
                    return false;

                }

                break;
            case command::FASTIC_SYNCRESET:

                if (dir == direction::GET) {
                    if(params[0] == '1'){
                        printf("FastIC 1 synchronous reset is %s\n\r", fastic::getFastIC1SyncReset() ? "high" : "low");
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 synchronous reset is %s\n\r", fastic::getFastIC2SyncReset() ? "high" : "low");
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }

                } else {
                    if(params[0] == '1'){
                        if(params[2] == 'h'){
                            fastic::setFastIC1SyncReset(1);
                            printf("FastIC 1 synchronous reset set to high\n\r");
                            return true;
                        } else if (params[2] == 'l'){
                            fastic::setFastIC1SyncReset(0);
                            printf("FastIC 1 synchronous reset set to low\n\r");
                            return true;
                        } else {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }
                    } else if (params[0] == '2'){
                        if(params[2] == 'h'){
                            fastic::setFastIC2SyncReset(1);
                            printf("FastIC 2 synchronous reset set to high\n\r");
                            return true;
                        } else if (params[2] == 'l'){
                            fastic::setFastIC2SyncReset(0);
                            printf("FastIC 2 synchronous reset set to low\n\r");
                            return true;
                        } else {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }
                    }
                    return false;
                }
                break;
            case command::FASTIC_CALPULSE:
                printf("This command is not implemented yet!\n\r");
                return false;
               /* if (dir == direction::GET) {

                    return false;
                    // Process set FastIC calibration pulse command
                } else {
                    // Invalid direction for this command
                    return false;
                }*/
                break;
            case command::FASTIC_TIME:
                if (dir == direction::GET) {
                    if(params[0] == '1'){
                        printf("FastIC 1 time ouptut is %s\n\r", fastic::getFastIC1Time() ? "high" : "low");
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 1 time ouptut is %s\n\r", fastic::getFastIC2Time() ? "high" : "low");
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }
                } else {
                    printf("This command does not support SET!\n\r");
                    return false;
                }
                break;
            case command::FASTIC_AURORA:
                printf("This command is not implemented yet!\n\r");
                return false;
                /*if (dir == direction::GET) {
                    // Process get FastIC aurora command
                } else {
                    // Invalid direction for this command
                    return false;
                }*/
                break;
            case command::USERBOARD_STATUS:
                if (dir == direction::GET) {
                    // Process get userboard status command
                } else {
                    // Invalid direction for this command
                    return false;
                }
                break;
            case command::USERBOARD_REGISTER:
                if (dir == direction::GET) {
                    // Process get userboard register command
                } else {
                    // Process set userboard register command
                } 
                break;
            case command::USERBOARD_ID:
                if (dir == direction::GET) {
                    // Process get userboard ID command
                } else {
                    // Invalid direction for this command
                    return false;
                }
                break;
            case command::USERBOARD_NAME:
                if (dir == direction::GET) {
                    // Process get userboard name command
                } else {
                    // Invalid direction for this command
                    return false;
                }
                break;
            case command::USERBOARD_VOLTAGE:
                if (dir == direction::GET) {
                    // Process get userboard voltage command
                } else {
                    // Process set userboard voltage command
                } 
                break;
            case command::USERBOARD_TOMEMORY:
                if (dir == direction::GET) {
                    // Process set userboard to memory command
                } else {
                    // Invalid direction for this command
                    return false;
                }
                break;
            case command::USERBOARD_FROMMEMORY:
                if (dir == direction::GET) {
                    // Process get userboard from memory command
                } else {
                    // Invalid direction for this command
                    return false;
                }
                break;

            default:
                // Unknown command
                return false;
        }

        return true;
    }
}