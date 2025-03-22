#ifndef FASTIC_H
#define FASTIC_H

#include<cstdint>

extern uint8_t fastic1_buffers[2][1024];

namespace fastic {

    enum class identifier {
        FASTIC1,
        FASTIC2
    };

    void init();
    void syncClock();
    void initInjectionChannels();
    bool getFastICSyncReset(identifier id);
    void setFastICSyncReset(identifier id, uint8_t value);
    bool getFastICTime(identifier id);

    uint8_t getFastICRegister(identifier id, uint8_t address);
    bool setFastICRegister(identifier id, uint8_t address, uint8_t value);

    bool registerRequireForce(uint8_t address);
    bool registerIsInRange(uint8_t address); 
    
}


#endif