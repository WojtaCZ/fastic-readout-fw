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

#endif
