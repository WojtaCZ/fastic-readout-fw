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
#include "userboard.hpp"

#include <errno.h>


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
        "usertboard init",
        "userboard uid",
        "userboard name",
        "userboard writeprotect",
        "userboard voltage",
        "userboard register",
        "userboard tomemory",
        "userboard frommemory"
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

    /*void sendStatus(){
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
    }*/

    bool parseHexNumber(char * string, char ** endptr, uint32_t &number){
        errno = 0;
        number = strtol(string, endptr, 16);
        if((*endptr == string) || ((number == 0) && (errno != 0))){
            return false;
        } else {
            return true;
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
                case 'u':
                    return command::USERBOARD_UID;
                case 'n':
                    return command::USERBOARD_NAME;
                case 'w':
                    return command::USERBOARD_WRITEPROTECT;
                case 'i':
                    return command::USERBOARD_INIT;
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
                    if(params[0] == '1'){

                        uint32_t address;
                        char * endptr;

                        if(!parseHexNumber(params + 2, &endptr, address)) {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }

                        if(!fastic::registerIsInRange(address)) {
                            printf("Register address out of range!\n\r");
                            return false;
                        }

                        printf("FastIC 1 register 0x%02x has a value of 0x%02x\n\r", address, fastic::getFastICRegister(fastic::identifier::FASTIC1, address));
                        return true;

                    } else if (params[0] == '2'){

                        uint32_t address;
                        char * endptr;

                        if(!parseHexNumber(params + 2, &endptr, address)) {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }

                        if(!fastic::registerIsInRange(address)) {
                            printf("Register address out of range!\n\r");
                            return false;
                        }

                        printf("FastIC 2 register 0x%02x has a value of 0x%02x\n\r", address, fastic::getFastICRegister(fastic::identifier::FASTIC2, address));
                        return true;

                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }
                } else {
                    if(params[0] == '1'){

                        uint32_t address, value;
                        char * endptr;
                        char * nextptr;

                        if(!parseHexNumber(params + 2, &nextptr, address)) {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }

                        if(!fastic::registerIsInRange(address)) {
                            printf("Register address out of range!\n\r");
                            return false;
                        }

                        if(!parseHexNumber(nextptr , &endptr, value)) {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }

                        if(value > 0xFF){
                            printf("Value out of range!\n\r");
                            return false;
                        }
                       
                        if(fastic::registerRequireForce(address) && endptr[1] != 'f'){
                            printf("This register can and probably will affect the function of the FastIC in a way that can break stream reception. Please modify this register carefully. The command needs to end with letter 'f' in order to force this register write.\n\r");
                            return false;
                        }
                        
                        fastic::setFastICRegister(fastic::identifier::FASTIC1, address, value);
                        printf("FastIC 1 register 0x%02x has been set to a value of 0x%02x\n\r", address, fastic::getFastICRegister(fastic::identifier::FASTIC1, address));
                        return true;

                    } else if (params[0] == '2'){
                        uint32_t address, value;
                        char * endptr;
                        char * nextptr;

                        if(!parseHexNumber(params + 2, &nextptr, address)) {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }

                        if(!fastic::registerIsInRange(address)) {
                            printf("Register address out of range!\n\r");
                            return false;
                        }

                        if(!parseHexNumber(nextptr , &endptr, value)) {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }

                        if(value > 0xFF){
                            printf("Value out of range!\n\r");
                            return false;
                        }
                       
                        if(fastic::registerRequireForce(address) && endptr[1] != 'f'){
                            printf("This register can and probably will affect the function of the FastIC in a way that can break stream reception. Please modify this register carefully. The command needs to end with letter 'f' in order to force this register write.\n\r");
                            return false;
                        }
                        
                        fastic::setFastICRegister(fastic::identifier::FASTIC2, address, value);
                        printf("FastIC 2 register 0x%02x has been set to a value of 0x%02x\n\r", address, fastic::getFastICRegister(fastic::identifier::FASTIC2, address));
                        return true;

                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }
                } 

                break;
            case command::FASTIC_VOLTAGE:

                if (dir == direction::GET) {

                    if(params[0] == '1'){
                        printf("FastIC 1 VMON voltage: %.2f [mV]\n\r", analog::getFastICVoltage(fastic::identifier::FASTIC1)*1000);
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 VMON voltage: %.2f [mV]\n\r", analog::getFastICVoltage(fastic::identifier::FASTIC2)*1000);
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
                        printf("FastIC 1 synchronous reset is %s\n\r", fastic::getFastICSyncReset(fastic::identifier::FASTIC1) ? "high" : "low");
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 synchronous reset is %s\n\r", fastic::getFastICSyncReset(fastic::identifier::FASTIC2) ? "high" : "low");
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }

                } else {
                    if(params[0] == '1'){
                        if(params[2] == 'h'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC1, 1);
                            printf("FastIC 1 synchronous reset set to high\n\r");
                            return true;
                        } else if (params[2] == 'l'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC1, 0);
                            printf("FastIC 1 synchronous reset set to low\n\r");
                            return true;
                        } else {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }
                    } else if (params[0] == '2'){
                        if(params[2] == 'h'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC2, 1);
                            printf("FastIC 2 synchronous reset set to high\n\r");
                            return true;
                        } else if (params[2] == 'l'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC2, 0);
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
                        printf("FastIC 1 time ouptut is %s\n\r", fastic::getFastICTime(fastic::identifier::FASTIC1) ? "high" : "low");
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 time ouptut is %s\n\r", fastic::getFastICTime(fastic::identifier::FASTIC2) ? "high" : "low");
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
                userboard::writeByte(0, 0xFF);
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
                    {
                        uint8_t shortID = userboard::getShortID();
                        std::vector<uint8_t> longID;
    
                        if(shortID == 0x0){
                            printf("Userboard is not connected!\n\r");
                            return false;
                        }
    
                       /* if(shortID != 0xF){
                            printf("Userboard does not contain an EEPROM!\n\r");
                            return false;
                        }

                        if(!userboard::isInitialized()){
                            printf("Userboard is not initialized! Please initialize it first.\n\r");
                            return false;
                        }
    
                        if(!userboard::getUID(longID)){
                            printf("Could not get userboard UID.\n\r");
                        }*/

                        if(shortID != 0xF){
                            sprintf(printBuffer, "Userboard status:\n\r   EEPROM: not present\n\r   Short ID: 0x%01X\n\r\n\r",
                                shortID
                            );
        
                            tud_cdc_write(printBuffer, strlen(printBuffer));
                            tud_cdc_write_flush();
                        }else{

                            if(!userboard::isInitialized()){
                                sprintf(printBuffer, "Userboard status:\n\r   EEPROM: present, not initialized\n\r   Short ID: 0x%01X\n\r",
                                    shortID
                                );
                            }else{

                                userboard::memoryHeader header;
                                userboard::readHeader(header);

                                sprintf(printBuffer, "Userboard status:\n\r   EEPROM: present, initialized\n\r   Short ID: 0x%01X\n\r   Userboard UID: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n\r   Memory write cycles: %d\n\r   Memory write protect: %s\n\r   Userboard name: %s\n\r",
                                    shortID,
                                    header.uid[0], header.uid[1], header.uid[2], header.uid[3],
                                    header.uid[4], header.uid[5], header.uid[6], header.uid[7],
                                    header.uid[8], header.uid[9], header.uid[10], header.uid[11],
                                    header.uid[12], header.uid[13], header.uid[14], header.uid[15],
                                    header.writeCycles,
                                    header.writeProtect ? "true" : "false",
                                    header.nameInited ? header.name : "not set"
                                );
                
                                tud_cdc_write(printBuffer, strlen(printBuffer));
                                tud_cdc_write_flush();
                            }
                        }
                        return true;
                    
                    }
                } else {
                    printf("This command does not support SET!\n\r");
                    return false;
                }
                break;
            case command::USERBOARD_UID:
                if (dir == direction::GET) {
                    {
                        uint8_t shortID = userboard::getShortID();
                        std::vector<uint8_t> longID;
    
                        if(shortID == 0x0){
                            printf("Userboard is not connected!\n\r");
                            return false;
                        }
    
                        if(shortID != 0xF){
                            printf("Userboard does not contain an EEPROM!\n\r");
                            return false;
                        }

                        if(!userboard::isInitialized()){
                            printf("Userboard is not initialized! Please initialize it first.\n\r");
                            return false;
                        }
    
                        if(userboard::getUID(longID)){
                            sprintf(printBuffer, "Userboard short ID: 0x%01X (EEPROM present)\n\rUserboard UID: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n\r",
                                shortID,
                                longID.at(0), longID.at(1), longID.at(2), longID.at(3),
                                longID.at(4), longID.at(5), longID.at(6), longID.at(7),
                                longID.at(8), longID.at(9), longID.at(10), longID.at(11),
                                longID.at(12), longID.at(13), longID.at(14), longID.at(15)
                            );
        
                            tud_cdc_write(printBuffer, strlen(printBuffer));
                            tud_cdc_write_flush();
                        } else {
                            printf("Could not get userboard UID.\n\r");
                        }

                        return true;
                    
                    }

                    // Process get userboard ID command
                } else {
                    printf("This command does not support SET!\n\r");
                    return false;
                }
                break;
            case command::USERBOARD_NAME:
                {
                    uint8_t shortID = userboard::getShortID();

                    if(shortID == 0x0){
                        printf("Userboard is not connected!\n\r");
                        return false;
                    }

                    if(shortID != 0xF){
                        printf("Userboard does not contain an EEPROM!\n\r");
                        return false;
                    }
                }

                if(!userboard::isInitialized()){
                    printf("Userboard is not initialized! Please initialize it first.\n\r");
                    return false;
                }

                if (dir == direction::GET) {
                    if(userboard::getName(printBuffer, 64)){
                        printf("Userboard name: %s\n\r", printBuffer);
                    } else {
                        printf("Userboard name is not set!\n\r");
                    }

                    // Process get userboard name command
                } else {
                    // Terminate with zero character
                    params[strlen(params) - 2] = '\0';
                    if(userboard::setName(params, strlen(params) + 1)){
                        printf("Userboard name set to: %s\n\r", params);
                    } else {
                        printf("Could not set the userboard name!\n\r");
                    }

                    return true;
                }
                break;

            case command::USERBOARD_WRITEPROTECT:
                {
                    uint8_t shortID = userboard::getShortID();

                    if(shortID == 0x0){
                        printf("Userboard is not connected!\n\r");
                        return false;
                    }

                    if(shortID != 0xF){
                        printf("Userboard does not contain an EEPROM!\n\r");
                        return false;
                    }
                }

                if(!userboard::isInitialized()){
                    printf("Userboard is not initialized! Please initialize it first.\n\r");
                    return false;
                }

                if (dir == direction::GET) {
                    printf("Userboard write protect: %s\n\r", userboard::getWriteProtect() ? "true" : "false");
                } else {
                    if(params[0] == 't'){
                        userboard::setWriteProtect(true);
                        printf("Userboard write protect set to true\n\r");
                        return true;
                    } else if (params[0] == 'f'){
                        userboard::setWriteProtect(false);
                        printf("Userboard write protect set to false\n\r");
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }
                } 
                
                break;

            case command::USERBOARD_INIT:
                {
                    uint8_t shortID = userboard::getShortID();

                    if(shortID == 0x0){
                        printf("Userboard is not connected!\n\r");
                        return false;
                    }

                    if(shortID != 0xF){
                        printf("Userboard does not contain an EEPROM!\n\r");
                        return false;
                    }
                }

                if (dir == direction::GET) {
                    if(userboard::isInitialized()){
                        printf("Userboard is already initialized.\n\r");
                        return true;
                    } else {
                        printf("Userboard is not initialized.\n\r");
                        return true;
                    }
                } else {
                    if(userboard::initHeader()){
                        printf("Userboard initialized successfully!\n\r");
                        return true;
                    } else {
                        printf("Userboard is already initialized!\n\r");
                        return false;
                    }
                } 
                break;
            case command::USERBOARD_VOLTAGE:
                if (dir == direction::GET) {
                    // Process get userboard voltage command
                } else {
                    // Process set userboard voltage command
                } 
                break;
            case command::USERBOARD_REGISTER:
                if (dir == direction::GET) {
                    // Process get userboard register command
                } else {
                    // Process set userboard register command
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

    bool processBinaryCommand(command cmd, direction dir, char * params, uint32_t * length){
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
                    float current = hv::getCurrent();
                    memcpy(params, &current, sizeof(float));
                    *length = sizeof(float);

                    return true;
                } else {
                    return false;
                }

                break;
            case command::HV_VOLTAGE:

                if (dir == direction::GET) {
                    float voltage = hv::getVoltage();
                    memcpy(params, &voltage, sizeof(float));
                    *length = sizeof(float);

                    return true;
                } else {
                    float voltage = std::bit_cast<float>(params[0] | (params[1] << 8) | (params[2] << 16) | (params[3] << 24));

                    if(voltage < 0 || voltage > 80){
                        return false;
                    }
                    
                    hv::setVoltage(voltage);

                    return true;
                }

                break;
            case command::FASTIC_REGISTER:

                if (dir == direction::GET) {
                    
                } else {
                    // Process set FastIC register command
                } 

                break;
            case command::FASTIC_VOLTAGE:

                if (dir == direction::GET) {

                    if(params[0] == '1'){
                        printf("FastIC 1 VMON voltage: %.3f [V]\n\r", analog::getFastICVoltage(fastic::identifier::FASTIC1));
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 VMON voltage: %.3f [V]\n\r", analog::getFastICVoltage(fastic::identifier::FASTIC2));
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
                        printf("FastIC 1 synchronous reset is %s\n\r", fastic::getFastICSyncReset(fastic::identifier::FASTIC1) ? "high" : "low");
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 2 synchronous reset is %s\n\r", fastic::getFastICSyncReset(fastic::identifier::FASTIC2) ? "high" : "low");
                        return true;
                    } else {
                        printf("Invalid parameter!\n\r");
                        return false;
                    }

                } else {
                    if(params[0] == '1'){
                        if(params[2] == 'h'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC1, 1);
                            printf("FastIC 1 synchronous reset set to high\n\r");
                            return true;
                        } else if (params[2] == 'l'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC1, 0);
                            printf("FastIC 1 synchronous reset set to low\n\r");
                            return true;
                        } else {
                            printf("Invalid parameter!\n\r");
                            return false;
                        }
                    } else if (params[0] == '2'){
                        if(params[2] == 'h'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC2, 1);
                            printf("FastIC 2 synchronous reset set to high\n\r");
                            return true;
                        } else if (params[2] == 'l'){
                            fastic::setFastICSyncReset(fastic::identifier::FASTIC2, 0);
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
                        printf("FastIC 1 time ouptut is %s\n\r", fastic::getFastICTime(fastic::identifier::FASTIC1) ? "high" : "low");
                        return true;
                    } else if (params[0] == '2'){
                        printf("FastIC 1 time ouptut is %s\n\r", fastic::getFastICTime(fastic::identifier::FASTIC2) ? "high" : "low");
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
                    return true;
                }
                break;
            case command::USERBOARD_REGISTER:
                if (dir == direction::GET) {
                    // Process get userboard register command
                } else {

                    return true;
                    // Process set userboard register command
                } 
                break;
            case command::USERBOARD_UID:
                if (dir == direction::GET) {
                    // Process get userboard ID command
                } else {

                    return false;
                    // Invalid direction for this command

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

            #ifdef DEBUG_COMMANDS

            case command::HV_PID:
                if (dir == direction::GET) {

                    float PID[3];
                    hv::getPID(PID[0], PID[1], PID[2]);

                    memcpy(params, PID, sizeof(float)*3);
                    *length = sizeof(float)*3;

                    return true;

                } else {

                    float P = std::bit_cast<float>(params[0] | (params[1] << 8) | (params[2] << 16) | (params[3] << 24));
                    float I = std::bit_cast<float>(params[4] | (params[5] << 8) | (params[6] << 16) | (params[7] << 24));
                    float D = std::bit_cast<float>(params[8] | (params[9] << 8) | (params[10] << 16) | (params[11] << 24));

                    hv::setPID(P, I, D);
                    return true;
                }

                break;
            
                /*case command::USERBOARD_REGISTER:
                    if (dir == direction::GET) {
                        // Process get userboard register command
                    } else {
                        // Process set userboard register command
                    }

                break;*/

            #endif
            default:
                // Unknown command
                return false;
        }

        return true;
    }
}