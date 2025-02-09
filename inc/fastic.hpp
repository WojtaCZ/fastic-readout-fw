#ifndef FASTIC_H
#define FASTIC_H

#include<cstdint>

extern uint8_t fastic1_buffers[2][1024];

namespace fastic {
    void init();
    void syncClock();
    void initInjectionChannels();

    
}


#endif