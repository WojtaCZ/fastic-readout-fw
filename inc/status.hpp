#ifndef STATUS_H
#define STATUS_H

#include <cstdint>

namespace status{
    void init();
    void process();

    namespace error{
        void set();
        void reset();
    }
}

#endif
