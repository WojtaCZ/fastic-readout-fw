#ifndef REGISTER_H
#define REGISTER_H

#include <cstdint>
#include <cstddef>

namespace reg{
    template<uint32_t address>
    void write(uint32_t value){
        *reinterpret_cast<volatile uint32_t *>(address) = value;
    }

    template<uint32_t address>
    uint32_t read(){
        return *reinterpret_cast<volatile uint32_t *>(address);
    }

    template<uint32_t address>
    uint32_t read(uint32_t mask){
        return (*reinterpret_cast<volatile uint32_t *>(address)) & mask;
    }

    template<uint32_t address>
    void set(uint32_t mask){
        *reinterpret_cast<volatile uint32_t *>(address) |= mask; 
    }

    template<uint32_t address>
    void clear(uint32_t mask){
        *reinterpret_cast<volatile uint32_t *>(address) &= ~mask;
    }

    template<uint32_t address>
    void toggle(uint32_t mask){
        *reinterpret_cast<volatile uint32_t *>(address) ^= mask;
    }
}

#endif
