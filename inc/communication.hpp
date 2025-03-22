#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <array>
#include <cstdint>

#define DEBUG_COMMANDS

namespace communication {

    enum class direction {
        SET,
        GET,
        UNKNOWN
    };

    enum class command {
        READOUT_STATUS,
        READOUT_ID,
        HV_ENABLE,
        HV_CURRENT,
        HV_VOLTAGE,
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

        #ifdef DEBUG_COMMANDS
        HV_PID,
        #endif

        UNKNOWN
    };
    


    command parseTextCommand(char *command);
    direction parseDirection(char * textCommand);
    bool processTextCommand(command &cmd, direction &dir, char * params);
    bool processBinaryCommand(command cmd, direction dir, char * params, uint32_t * length);

    void process();
    void sendStatus();


}

#endif