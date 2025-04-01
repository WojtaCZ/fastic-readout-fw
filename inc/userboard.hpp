#ifndef USERBOARD_H
#define USERBOARD_H

#include <cstdint>
#include <vector>


namespace userboard {

    // Define the memory structure of the eeprom
    struct memoryHeader {
        uint8_t uid[16];
        uint32_t writeCycles;
        uint8_t writeProtect : 1;
        uint8_t uidInited : 1;
        uint8_t nameInited : 1;
        uint8_t voltageInited : 1;
        uint8_t fastic1Inited : 1;
        uint8_t fastic2Inited : 1;
        uint8_t reservedStatus : 2;
        uint8_t reserved[16];
        char name[64];
        float voltage;
    } __attribute__((packed));

    struct memory {
        memoryHeader header;
        float voltage;
        uint8_t fastic1_registers[190];
        uint8_t fastic2_registers[190];
    } __attribute__((packed));

    static_assert(sizeof(memory) < 512, "The memory structure does not fit into the EEPROM");

    bool getName(char * name, uint8_t size);
    bool setName(char * name, uint8_t size);

    bool getWriteProtect();
    bool setWriteProtect(bool protect);

    bool getUID(std::vector<uint8_t> & UID);
    uint8_t getShortID();

    bool readHeader(memoryHeader & header);
    bool writeHeader(memoryHeader & header);
    bool initHeader();
    bool isInitialized();

    bool readByte(uint16_t address, uint8_t & data);
    bool readBlock(uint16_t address, uint16_t size, std::vector<uint8_t> & data);

    bool writeByte(uint16_t address, uint8_t data);
    bool writeBlock(uint16_t address, std::vector<uint8_t> data);

}

#endif