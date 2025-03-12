#ifndef FASTIC_H
#define FASTIC_H

#include<cstdint>

extern uint8_t fastic1_buffers[2][1024];

namespace fastic {
    void init();
    void syncClock();
    void initInjectionChannels();
    bool getFastIC1SyncReset();
    bool getFastIC2SyncReset();
    void setFastIC1SyncReset(uint8_t value);
    void setFastIC2SyncReset(uint8_t value);
    bool getFastIC1Time();
    bool getFastIC2Time();
    
}


#endif