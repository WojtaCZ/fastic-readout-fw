#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <array>
#include <cstdint>

namespace communication {

    enum class direction {
        GET,
        SET,
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
        USERBOARD_REGISTER,
        USERBOARD_ID,
        USERBOARD_NAME,
        USERBOARD_VOLTAGE,
        USERBOARD_TOMEMORY,
        USERBOARD_FROMMEMORY,
        UNKNOWN
    };
    


    command parseTextCommand(char *command);
    direction parseDirection(char * textCommand);
    bool processTextCommand(command &cmd, direction &dir, char * params);

    void process();
    void sendStatus();


}

#endif