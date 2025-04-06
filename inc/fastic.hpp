#ifndef FASTIC_H
#define FASTIC_H

#include<cstdint>

namespace fastic::common {
    void init();
    bool registerRequireForce(uint8_t address);
    bool registerIsInRange(uint8_t address); 
}

namespace fastic1 {
    bool init();
    void enableStream();
    void disableStream();
    bool isStreaming();
    void enableForceWordMode(uint32_t word);
    void disableForceWordMode();
    void enableInjection();
    void disableInjection();
    bool isInjecting();
    bool getSyncReset();
    void setSyncReset(bool value);
    bool getTime();
    uint8_t getRegister(uint8_t regAddress);
    bool setRegister(uint8_t regAddress, uint8_t value);

}


namespace fastic2 {
    bool init();
    void enableStream();
    void disableStream();
    bool isStreaming();
    void enableForceWordMode(uint32_t word);
    void disableForceWordMode();
    void enableInjection();
    void disableInjection();
    bool isInjecting();
    bool getSyncReset();
    void setSyncReset(bool value);
    bool getTime();
    uint8_t getRegister(uint8_t regAddress); 
    bool setRegister(uint8_t regAddress, uint8_t value);
}


#endif