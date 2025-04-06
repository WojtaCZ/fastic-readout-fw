#ifndef BOARD_H
#define BOARD_H
#include <cstdint>

namespace memory {
    void disableCachingD2();
    void disableCachingD3();
    void enableMPU();
}

namespace rng {
    void init();
    uint32_t get();
}

namespace board {
    enum class status {
        OK,
        STREAMING,
        ERROR
    };

    void processStatus();
    status getStatus();
    void setStatus(status s);
}

    
#endif
