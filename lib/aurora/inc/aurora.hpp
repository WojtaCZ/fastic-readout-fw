#ifndef AURORA_H
#define AURORA_H

#include <cstdint>
#include <vector>

namespace aurora{

    class packet;

    class rx
    {
        private:
            //Signal if the receiver has acquired sync
            bool synced_ = false;
            //What is the bitslip needed to get to sync
            std::uint8_t bitSlip_ = 0;

            //Index of the packet to retreive by the getPacket function
            std::uint16_t packetIdx_ = 0;

            //Pointer to bitstream with the raw data
            std::uint32_t * rxBuffer_;
            std::size_t rxBufferSize_;

            //Buffer of received packets
            std::vector<packet> packetBuffer_;

            //Calculate the bit slip from the stream
            std::uint8_t calculateBitSlip();
            bool validateSync(std::uint16_t packetIndex, std::uint8_t expectedSlip);
            std::uint8_t getSyncBits(std::uint16_t packetIndex, std::uint8_t expectedSlip);

            const int syncPacketCount_;
            const int syncErrorTreshold_;

            //Counter to keep the overall ber of the receiver
            float berCounter_ = 0;

        public:
            std::uint8_t calculateBitSlipTest();
            rx(std::uint32_t * rxBuffer, std::size_t rxBufferSize, const int syncPacketCount = 64, const int syncErrorTreshold = 40);
            //~rx();

            bool trySync();
            bool isSynchronized();

            void processBuffer(bool discardControl = false);
            std::uint8_t getBitSlip();
            void forceBitSlip(uint8_t bitslip);
            packet getPacket(bool autoIncrement);
            float getBER();
            const std::vector<packet> & getPacketBuffer() const;
    };

    class packet
    {
        public:
            enum class type{
                data = 0b01,    //Data frame preamble
                control = 0b10,  //Control frame preamble
                error
            };

        private:
            type packetType_;
            std::uint64_t packetData_;
        public:
            packet(type packetType, std::uint64_t packetData);
            //~packet();
            type getType();
            std::uint64_t getData();
    };

    class dataPacket : public packet{

    };

    class controlPacket : public packet{
        public:
            enum class btf{
                idle = 0x78,    //Idle, not ready or clock compensation
                nfc = 0xaa,     //Native Flow Control
                ufc = 0x2d,     //User Flow Control
                sep = 0x1e,     //Separator
                sep7 = 0xe1,    //Separator 7
                k0 = 0xd2,      //User K-Block 0
                k1 = 0x99,      //User K-Block 1
                k2 = 0x55,      //User K-Block 2
                k3 = 0xb4,      //User K-Block 3
                k4 = 0xcc,      //User K-Block 4
                k5 = 0x66,      //User K-Block 5
                k6 = 0x33,      //User K-Block 6
                k7 = 0x4b,      //User K-Block 7
                k8 = 0x87,      //User K-Block 8
                res = 0xff      //Reserved
            };

        private:
            btf packetBtf_;

        public:
            controlPacket(btf packetBtf, std::uint64_t packetData);
            //~controlPacket();
            btf getBtf();
    };   
}

#endif
