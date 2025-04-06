#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>


namespace communication {

    enum class direction {
        SET,
        GET,
        UNKNOWN
    };

    enum class command {
        READOUT_STATUS,
        READOUT_UID,
        HV_ENABLE,
        HV_CURRENT,
        HV_VOLTAGE,
        HV_PID,
        FASTIC_REGISTER,
        FASTIC_VOLTAGE,
        FASTIC_SYNCRESET,
        FASTIC_CALPULSE,
        FASTIC_TIME,
        FASTIC_AURORA,
        USERBOARD_STATUS,
        USERBOARD_INIT,
        USERBOARD_UID,
        USERBOARD_NAME,
        USERBOARD_WRITEPROTECT,
        USERBOARD_VOLTAGE,
        USERBOARD_REGISTER,
        USERBOARD_TOMEMORY,
        USERBOARD_FROMMEMORY,
        UNKNOWN
    };
    


    command parseTextCommand(char *command);
    direction parseDirection(char * textCommand);
    bool processTextCommand(command &cmd, direction &dir, char * params);
    bool processBinaryCommand(command cmd, direction dir, uint16_t index, uint16_t value, char * params, uint32_t * length);

    void process();
    void sendStatus();


}

#endif