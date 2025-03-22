#include "userboard.hpp"
#include "board.hpp"
#include <stmcpp/rng.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/i2c.hpp>
#include <stmcpp/register.hpp>


namespace userboard {
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 0> id0_sda(stmcpp::gpio::mode::input, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::low, stmcpp::gpio::pull::pullDown);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 1> id1_scl(stmcpp::gpio::mode::input, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::low, stmcpp::gpio::pull::pullDown);
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 12> id2(stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullDown);
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 13> id3(stmcpp::gpio::mode::input, stmcpp::gpio::pull::pullDown);
    
    static memoryHeader userboardHeader; 

    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c2> eeprom_i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    
    bool getWriteProtect(){
        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        // Read the header data
        readHeader(userboardHeader);    

        return userboardHeader.writeProtect;
    }

    bool setWriteProtect(bool protect){
        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        // Read the header data
        readHeader(userboardHeader);    

        userboardHeader.writeProtect = protect;

        // Write the header to the EEPROM
        writeHeader(userboardHeader);

        return true;
    }

    bool getName(char * name, uint8_t size){
        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        // Read the header data
        readHeader(userboardHeader);    

        // If the name is not inited, return false
        if(!userboardHeader.nameInited) return false;

        // Copy the name to the buffer
        for(int i = 0; i < size; i++){
            name[i] = userboardHeader.name[i];
        }

        return true;
    }

    bool setName(char * name, uint8_t size){
        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        // Read the header data
        readHeader(userboardHeader);    

        // Copy the name to the buffer
        for(int i = 0; i < size; i++){
            userboardHeader.name[i] = name[i];
        }

        userboardHeader.nameInited = 1;

        // Write the header to the EEPROM
        writeHeader(userboardHeader);

        return true;
    }

    bool getUID(std::vector<uint8_t> & UID){

        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        // Read the header data
        readHeader(userboardHeader);

        // If the UID is not inited, return false
        if(!userboardHeader.uidInited) return false;

        // Load the UID into the vector
        UID = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&userboardHeader.uid), reinterpret_cast<uint8_t*>(&userboardHeader.uid) + sizeof(userboardHeader.uid));

        return true;
    }

    uint8_t getShortID(){

        eeprom_i2c.disable();
        id0_sda.setMode(stmcpp::gpio::mode::input);
        id1_scl.setMode(stmcpp::gpio::mode::input);
        id0_sda.setPull(stmcpp::gpio::pull::pullDown);
        id1_scl.setPull(stmcpp::gpio::pull::pullDown);

        uint8_t id = (id0_sda.read() << 0) | (id1_scl.read() << 1) | (id2.read() << 2) | (id3.read() << 3); 

        return id;
    }

    bool isInitialized(){
        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        // Read the header data
        readHeader(userboardHeader);    
        
        // The default state of the EEPROM is 0xFF, if the header is not 0xFF, it is already initialized
        for(int i = 0; i < sizeof(userboardHeader); i++){
            if(reinterpret_cast<uint8_t*>(&userboardHeader)[i] != 0xFF) return true;
        }

        return false;
    }

    bool initHeader(){

        // If there is no eeprom present, return false
        if(getShortID() != 0xF) return false;

        if(isInitialized()) return false;

        memoryHeader header;

        // Generate a random UID
        stmcpp::rng::enable();

        uint32_t rndByte;

        for(int i = 0; i < 4; i++){
            rndByte = stmcpp::rng::getData();
            header.uid[0 + i*4] = (rndByte >> 24) & 0xFF;
            header.uid[1 + i*4] = (rndByte >> 16) & 0xFF;
            header.uid[2 + i*4] = (rndByte >> 8) & 0xFF;
            header.uid[3 + i*4] = rndByte & 0xFF;
        }

        stmcpp::rng::disable();     
        
        // Clear out the name field
        for(int i = 0; i < sizeof(header.name); i++){
            header.name[i] = 0;
        }

        // Zero out the write cycles
        header.writeCycles = 0;

        // Set all the othe necessary fields
        header.writeProtect = 0;
        header.uidInited = 1;
        header.nameInited = 0;
        header.voltageInited = 0;
        header.fastic1Inited = 0;
        header.fastic2Inited = 0;
        header.reservedStatus = 0;

        // Write the header to the EEPROM
        writeHeader(header);

        return true;

    }

    bool readHeader(memoryHeader & header){
        std::vector<uint8_t> data;
        readBlock(0, sizeof(memoryHeader), data);
        /*int idx = 0;
        for(uint8_t byte : data){
            ((uint8_t *)header + idx) = byte;
            idx++;
        }*/

        header = *reinterpret_cast<memoryHeader*>(data.data());
        return true;
    }

    bool writeHeader(memoryHeader & header){
        header.writeCycles++;
        writeBlock(0, std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&header), reinterpret_cast<uint8_t*>(&header) + sizeof(header)));
        return true;
    }
    
    bool readByte(uint16_t address, uint8_t & data){
        // Check the bounds
        if (address > 511) return false;

        id0_sda.setMode(stmcpp::gpio::mode::af4);
        id1_scl.setMode(stmcpp::gpio::mode::af4);
        id0_sda.setPull(stmcpp::gpio::pull::noPull);
        id1_scl.setPull(stmcpp::gpio::pull::noPull);
        eeprom_i2c.enable();

        // Control byte for the EEPROM read operation
        uint8_t controlByte = 0b1010000;   

        // If the address is greater than 255, change the block flag
        if(address > 255) controlByte |= 0b0000001;

        // The control byte becomes the I2C address
        stmcpp::i2c::address eeprom_address (controlByte);

        // Read the data from the EEPROM
        eeprom_i2c.write(address % 255, eeprom_address);
        data = eeprom_i2c.read(eeprom_address);
        
        return true;
    }


    bool readBlock(uint16_t address, uint16_t size, std::vector<uint8_t> & data){
        // Check the bounds
        if ((address + size) > 511) return false;

        id0_sda.setMode(stmcpp::gpio::mode::af4);
        id1_scl.setMode(stmcpp::gpio::mode::af4);
        id0_sda.setPull(stmcpp::gpio::pull::noPull);
        id1_scl.setPull(stmcpp::gpio::pull::noPull);
        eeprom_i2c.enable();

        // Control byte for the EEPROM read operation
        uint8_t controlByte = 0b1010000;   

        // If the address is greater than 255, change the block flag
        if(address > 255) controlByte |= 0b0000001;

        // The control byte becomes the I2C address
        stmcpp::i2c::address eeprom_address (controlByte);

        // Read the data from the EEPROM
        eeprom_i2c.write(address % 255, eeprom_address);
        eeprom_i2c.read(data, size, eeprom_address);
        
        return true;
    }


    bool writeByte(uint16_t address, uint8_t data){
        // Check the bounds
        if (address > 511) return false;

        id0_sda.setMode(stmcpp::gpio::mode::af4);
        id1_scl.setMode(stmcpp::gpio::mode::af4);
        id0_sda.setPull(stmcpp::gpio::pull::noPull);
        id1_scl.setPull(stmcpp::gpio::pull::noPull);
        eeprom_i2c.enable();

        // Read the header data
        readHeader(userboardHeader);

        // If the write protect flag is set, return false
        //řif(userboardHeader.writeProtect && userboard::isInitialized()) return false;

        // Control byte for the EEPROM read operation
        uint8_t controlByte = 0b1010000;

        // If the address is greater than 255, change the block flag
        if(address > 255) controlByte |= 0b0000001;

        // The control byte becomes the I2C address
        stmcpp::i2c::address eeprom_address (controlByte);

        // Write the data to the EEPROM
        eeprom_i2c.writeRegister(address % 255, data, eeprom_address);
        eeprom_i2c.stop();

        return true;
    }

    bool writeBlock(uint16_t address, std::vector<uint8_t> data){
        // Check the bounds
        if ((address + data.size()) > 511) return false;

        id0_sda.setMode(stmcpp::gpio::mode::af4);
        id1_scl.setMode(stmcpp::gpio::mode::af4);
        id0_sda.setPull(stmcpp::gpio::pull::noPull);
        id1_scl.setPull(stmcpp::gpio::pull::noPull);
        eeprom_i2c.enable();

        // Read the header data
        readHeader(userboardHeader);

        // If the write protect flag is set, return false
        if(userboardHeader.writeProtect && userboard::isInitialized()) return false;

        // Control byte for the EEPROM read operation
        uint8_t controlByte = 0b1010000;

        // The control byte becomes the I2C address
        stmcpp::i2c::address eeprom_address (controlByte);

        int timeout = 1000;

        // Write the data to the EEPROM
        for(uint8_t byte : data){
            if(address > 255) controlByte |= 0b0000001;
            eeprom_i2c.writeRegister(address % 255, byte, eeprom_address);
            eeprom_i2c.stop();
            address++;
    
            // Acknowledge polling - wait for the EEPROM to finish writing
            timeout = 10000;
            while(timeout > 0){
                eeprom_i2c.start();
                while (stmcpp::reg::read(std::ref(I2C2->ISR), I2C_ISR_TXIS_Msk | I2C_ISR_NACKF_Msk) == 0) {;}
                if(eeprom_i2c.getInterruptFlag(stmcpp::i2c::interrupt::txInterrupt)) break;
                timeout--;
            }

            if(timeout == 0) return false;
        }

        return true;
    }
}