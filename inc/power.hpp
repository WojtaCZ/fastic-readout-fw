#ifndef POWER_H
#define POWER_H

namespace power {
    enum class ldo {
        D1V2,
        T1V2,
        A1V2,
        A3V3,
        D1V8
    };

    bool isPowerGood(ldo regulator);
    void enable1V2();
    void disable1V2();
}

namespace hv{
    void init();
}
#endif
