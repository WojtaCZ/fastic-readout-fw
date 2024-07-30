#ifndef SI5340_H
#define SI5340_H

namespace si5340 {
    void init();
    void sendMagicPreamble();
    void sendMagicPostamble();
}

#endif