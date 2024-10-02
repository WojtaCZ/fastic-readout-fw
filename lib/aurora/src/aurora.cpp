#include "aurora.hpp"
//#include <iostream>
//#include <cstdio>
//#include <string>
#include <bitset>
#include <algorithm>
#include <cstdint>
//#include <iomanip>

namespace aurora{
    rx::rx(std::uint32_t * rxBuffer, std::size_t rxBufferSize, const int syncPacketCount, const int syncErrorTreshold) : 
        rxBuffer_(rxBuffer), 
        rxBufferSize_(rxBufferSize), 
        syncPacketCount_(syncPacketCount), 
        syncErrorTreshold_(syncErrorTreshold)
    { }


    std::uint8_t rx::getSyncBits(std::uint16_t packetIndex, std::uint8_t expectedSlip){
        //Calculate at which index in the data array should the sync bits be
        std::uint16_t syncIdx = (expectedSlip + 66*packetIndex) / 32;
        //Calculate the bitshift of the sync bits in the 32bit word
        std::uint8_t syncShift = (expectedSlip + 66*packetIndex) % 32;
        std::uint8_t syncBits;
        
        if(syncShift != 31){
            //Separate the sync bits
            syncBits = (rxBuffer_[syncIdx] & (0xC0000000 >> syncShift)) >> (30 - syncShift);
        }else{
            //The bits are split in the two packets
            syncBits = ((rxBuffer_[syncIdx] & 0x00000001) << 1) | ((rxBuffer_[syncIdx+1] & 0x80000000) >> 31);
        }

        return syncBits;
    }

    std::uint8_t rx::calculateBitSlip(){ 
        //Number of packets to go through when calculating BER
        constexpr auto BER_PACKET_COUNT = 64;
        //After how many correct packets should the bitslip be considered in sync
        constexpr auto SYNC_ERROR_TRESHOLD = 50;

        static_assert(unsigned(SYNC_ERROR_TRESHOLD) > 5, "Error treshold too low! This will lead to fake syncs. Decrease the treshold for more reliable sync.");

        //Counter for valid packets
        std::uint8_t validCounter;
        
        //Test all the possible bitslips
        for(std::uint8_t slip = 0; slip < 64; slip++){
            //Check the first 5 packets for validity (this usually gets rid of most of the fake syncs)
            if(validateSync(0, slip) && validateSync(1, slip) && validateSync(2, slip) && validateSync(3, slip) && validateSync(4, slip)){
                //If sync candidates were acquired, check their BER
                validCounter = 5;
                //Go through all the packet with the selected slip
                for(int packet = 5; packet < BER_PACKET_COUNT; packet++){
                    //Count how many of them were valid
                    if(validateSync(packet, slip)){
                        validCounter++;
                        //If the BER was good enough, return the slip value
                        if(validCounter > SYNC_ERROR_TRESHOLD) return slip;
                    }
                }
            }
        }
        
        return -1;
    }

     std::uint8_t rx::calculateBitSlipTest(){ 
       

        //Counter for valid packets
        int valids[64];
        int validCounter = 0, validCounterMax = -1;
        std::uint8_t max = 0;
        
        //Test all the possible bitslips
        for(std::uint8_t slip = 0; slip < 64; slip++){
            //Check the first 5 packets for validity (this usually gets rid of most of the fake syncs)
                for(int packet = 0; packet < (rxBufferSize_*32)/66 - 1; packet++){
                    //Count how many of them were valid
                    if(validateSync(packet, slip)){
                        validCounter++;
                    }
                }
                

                if(validCounter >= validCounterMax){  //// CHANGE THIS!!!!
                    max = slip;
                    validCounterMax = validCounter;
                }
                //std::cout << "Valid packets for slip " << std::dec << std::setw(6) << std::setfill(' ') << unsigned(slip) << " are " << std::dec << std::setw(6) << std::setfill(' ') << unsigned(validCounter) << std::endl;
                valids[slip] = validCounter;
                validCounter = 0;
                
        }


        return max;
    }

    //Validate the sync bytes at specified position
    bool rx::validateSync(std::uint16_t packetIndex, std::uint8_t expectedSlip){
        //Calculate at which index in the data array should the sync bits be
        std::uint16_t syncIdx = (expectedSlip + 66*packetIndex) / 32;
        //Calculate the bitshift of the sync bits in the 32bit word
        std::uint8_t syncShift = (expectedSlip + 66*packetIndex) % 32;
        //Separate the sync bits
        std::uint8_t syncBits = getSyncBits(packetIndex, expectedSlip);

        //If the sync bits are incorrect, return false
        if(syncBits == 0b00) return false;
        if(syncBits == 0b11) return false;
        
        return true;
    }

    //Try to sync the receiver to the data stream
    bool rx::trySync(){
        //Figure out the bitslip
        //this->bitSlip_ = calculateBitSlip();
        this->bitSlip_ = calculateBitSlipTest();
        
        //If the sync headers were detected, flag sync
        if(this->bitSlip_ != 255){
            this->synced_ = true;
            this->berCounter_ = 0;
        }else{
            this->synced_ = false;
        }

        return this->synced_;
    }

    //Return the current bitslip
    std::uint8_t rx::getBitSlip(){
        return this->bitSlip_;
    }

    void rx::forceBitSlip(uint8_t bitslip){
        this->bitSlip_ = bitslip;
    }

    //Return the overall BER of the receiver
    float rx::getBER(){
        return berCounter_;
    }

    packet rx::getPacket(bool autoIncrement){
        //Calculate at which index in the data array should the sync bits be
        std::uint16_t syncIdx = (this->bitSlip_ + 66*this->packetIdx_) / 32;
        //Calculate the bitshift of the sync bits in the 32bit word
        std::uint8_t syncShift = (this->bitSlip_ + 66*this->packetIdx_) % 32;
        //Get the synchronization bits
        std::uint8_t syncBits = getSyncBits(this->packetIdx_, this->bitSlip_);

        //Sort out the packet type
        packet::type packetType;

        if(syncBits == 0b01){
            packetType = packet::type::data;
        }else if(syncBits == 0b10){
            packetType = packet::type::control;
        }else{
            packetType = packet::type::error;
        }

        std::uint64_t data;

        //Reconstruct the packet data
        if(syncShift == 31){
            //If the bitslip is 31 its a special case
            data = 0
                   | ((std::uint64_t)(rxBuffer_[syncIdx+1] & 0x7FFFFFFF) << 33)
                   | ((std::uint64_t) rxBuffer_[syncIdx+2]               << 1)
                   | ((std::uint64_t)(rxBuffer_[syncIdx+3] & 0x80000000) >> 31);
        }else{
            data = 0
                 | ((std::uint64_t)((rxBuffer_[syncIdx]     & (0x3FFFFFFF >> syncShift)))           << (32 + syncShift + 2))
                 | ((std::uint64_t)  rxBuffer_[syncIdx+1])                                          << (syncShift + 2)
                 | ((std::uint64_t)((rxBuffer_[syncIdx+2]   & (0xFFFFFFFF << (30 - syncShift))))    >> (30 - syncShift));
        }

        if(autoIncrement) this->packetIdx_++;

        return packet(packetType, data);

    }

    void rx::processBuffer(bool discardControl){
        //Clear the buffer
        packetBuffer_.clear();
        this->packetIdx_ = 0;

        std::uint16_t treshold = 0;

        //If there is no bitslip, one more packet can be read
        if(bitSlip_){
            treshold = (rxBufferSize_*32)/66 - 1;
        }else treshold = (rxBufferSize_*32)/66;
        
        //Go through the buffer and retreive all available packets
        for(int idx = 0; idx < treshold; idx++){
            packet p = getPacket(true);
            if(p.getType() == packet::type::error) berCounter_++;
            if(!(discardControl && p.getType() == packet::type::control)){
                packetBuffer_.push_back(p);
            } 
        }

        //Divide the error frames by the number of frames received
        berCounter_ /= (treshold + 1);
    }

    //Return the reference to the internal buffer
    const std::vector<packet> & rx::getPacketBuffer() const{
        return packetBuffer_;
    }

    //Packet constructor
    packet::packet(type packetType, std::uint64_t packetData) : 
        packetType_(packetType), 
        packetData_(packetData)
    { }

    //Return the packet type
    packet::type packet::getType(){
        return packetType_;
    }

    //Return the packet data
    std::uint64_t packet::getData(){
        return packetData_;
    }   

    
}