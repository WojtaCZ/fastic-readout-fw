#ifndef SPI_H
#define SPI_H

#include "stm32h753xx.h"
#include <cstdint>

namespace spi{

    enum class peripheral : uint32_t {
        spi1 = SPI1_BASE,
        spi2 = SPI2_BASE,
        spi3 = SPI3_BASE,
        spi4 = SPI4_BASE,
        spi5 = SPI5_BASE,
        spi6 = SPI6_BASE
    };

    enum class role {
        slave   = 0b0, ///< SPI is configured as slave
        master  = 0b1  ///< SPI is configured as master
    };

    enum class mode {
        fullduplex  = 0b00, ///< SPI is configured in full duplex mode
        txsimplex   = 0b01, ///< SPI is configured in transmitter simplex mode
        rxsimplex   = 0b10, ///< SPI is configured in receiver simplex mode
        halfduplex  = 0b11 ///< SPI is configured in half duplex mode
    };

    enum class direction {
        receiver    = 0b0, ///< SPI half-dupex direction is receiver
        transmitter = 0b1  ///< SPI half-dupex direction is transmitter
    };


    enum class protocol {
        motorola    = 0b000, ///< SPI is configured to use the motorola protocol
        ti          = 0b001  ///< SPI is configured to use the TI protocol
    };

    enum class clockpol {
        idlelow     = 0b0, ///< The SPI clock is low in idle
        idlehigh    = 0b1  ///< The SPI clock is high in idle
    };

    enum class ssorigin {
        sspad       = 0b0, 
        ssbit       = 0b1  
    };

    enum class ssbehavior {
        endoftransfer       = 0b0, ///< SS is kept at active level till data transfer is completed, it becomes inactive with EOT flag
        interleaved         = 0b1  ///< SPI data frames are interleaved with SS non active pulses when dataIdleness > 1
    };

    enum class afbehavior {
        nocontrol           = 0b0, ///< The peripheral takes no control of GPIOs while it is disabled
        keepcontrol         = 0b1  ///< The peripheral keeps always control of all associated GPIOs
    };

    enum class clockphase {
        firsttransition     = 0b0, ///< The first clock transition is the first data capture edge
        secondtransition    = 0b1  ///< The second clock transition is the first data capture edge
    };

    enum class bitorder {
        msbfirst    = 0b0, ///< MSB is transmitted first
        lsbfirst    = 0b1  ///< LSB is transmitted first
    };

    enum class sspol {
        activelow   = 0b0, ///< The SPI slave select is active low
        activehigh  = 0b1  ///< The SPI slave select is active high
    };
    
    enum class underrunbehav {
        pattern             = 0b00, ///< Send out pattern defined by SPI_UDRDR
        repeatlastrx        = 0b01, ///< Repeat lastly received data
        repeatlasttx        = 0b10  ///< Repeat lastly transmitted data
    };

    enum class underrundet {
        beginframe          = 0b00, ///< Underrun detected at beginning of the data frame
        endframe            = 0b01, ///< Underrun detected at end of the data frame
        beginslaveselect    = 0b10  ///< Underrun detected at beginning of the slave select signal
    };

    enum class crc {
        allzero     = 0b0, ///< All zero calculation pattern 
        allone      = 0b1  ///< All one calculation patern
    };

    enum class masterdivider {
        div2        = 0b000, ///< The spi master clock is divided by 2
        div4        = 0b001, ///< The spi master clock is divided by 4
        div8        = 0b010, ///< The spi master clock is divided by 8
        div16       = 0b011, ///< The spi master clock is divided by 16
        div32       = 0b100, ///< The spi master clock is divided by 32
        div64       = 0b101, ///< The spi master clock is divided by 64
        div128      = 0b110, ///< The spi master clock is divided by 128
        div256      = 0b111  ///< The spi master clock is divided by 256
    };

    enum class interrupt : uint32_t {
        endoftransfer    = (0b1 << SPI_SR_EOT_Pos),   ///< End of transfer
        txfilled         = (0b1 << SPI_SR_TXTF_Pos),  ///< Transmission transfer filled
        underrun         = (0b1 << SPI_SR_UDR_Pos),   ///< Underrun
        overrun          = (0b1 << SPI_SR_OVR_Pos),   ///< Overrun
        crcerror         = (0b1 << SPI_SR_CRCE_Pos),  ///< CRC error
        tiferror         = (0b1 << SPI_SR_TIFRE_Pos), ///< TI frame format error
        modefault        = (0b1 << SPI_SR_MODF_Pos),  ///< Mode fault
        additionalreload = (0b1 << SPI_SR_TSERF_Pos), ///< Additional number of data
        suspend          = (0b1 << SPI_SR_SUSP_Pos),  ///< SPI suspended
        txcomplete       = (0b1 << SPI_SR_TXC_Pos),   ///< RX packet is available
        rxpacket         = (0b1 << SPI_SR_RXP_Pos),   ///< RX packet is available
        txpacket         = (0b1 << SPI_SR_TXP_Pos),   ///< TX packet space is available
        duplexpacket     = (0b1 << SPI_SR_DXP_Pos),   ///< RX packet was received and TX packet space is available
    };

    template<peripheral peripheral_, role role_, mode mode_, protocol protocol_, bitorder bitOrder_, clockpol clockPol_, clockphase clockPhase_, ssorigin ssOrigin_, sspol ssPol_, bool ssOutputEnable_, ssbehavior ssBehavior_, uint8_t frameSize_, uint8_t fifoTreshold_, masterdivider masterDivider_, 
            bool enableTXdma_ = false, bool enableRXdma_ = false, uint8_t ssIdleness_ = 0, uint8_t dataIdleness_ = 0, bool ioSwap_ = false, afbehavior afBehavior_ = afbehavior::nocontrol, bool crcEnable_ = false, uint8_t crcSize_ = 4, bool crcFullSize_ = false, crc crcRX_ = crc::allzero, crc crcTX_ = crc::allzero, bool enableSuspend_ = false, underrundet underrunDet_ = underrundet::beginframe, underrunbehav underrunBehav_ = underrunbehav::pattern>
    class spi{
        public:
            spi(){
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>( 
                    ((static_cast<uint8_t>(enableSuspend_) & 0b1) << SPI_CR1_MASRX_Pos) |
                    ((static_cast<uint8_t>(crcFullSize_) & 0b1) << SPI_CR1_CRC33_17_Pos) |
                    ((static_cast<uint8_t>(crcRX_) & 0b1) << SPI_CR1_RCRCINI_Pos) |
                    ((static_cast<uint8_t>(crcTX_) & 0b1) << SPI_CR1_TCRCINI_Pos)
                );

                static_assert(frameSize_ > 3 && frameSize_ < 33, "The number of bits has to be between 4 and 32!");
                static_assert(crcSize_ > 3 && crcSize_ < 31, "The CRC size has to be between 4 and 32 bits!");
                static_assert(frameSize_ < 17, "The FIFO treshold has to be less than 17!");

                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>( 
                    ((static_cast<uint8_t>(frameSize_ - 1) & 0b11111) << SPI_CFG1_DSIZE_Pos) |
                    ((static_cast<uint8_t>(fifoTreshold_ - 1) & 0b1111) << SPI_CFG1_FTHLV_Pos) |
                    ((static_cast<uint8_t>(underrunBehav_) & 0b11) << SPI_CFG1_UDRCFG_Pos) |
                    ((static_cast<uint8_t>(underrunDet_) & 0b11) << SPI_CFG1_UDRDET_Pos) |
                    ((static_cast<uint8_t>(enableRXdma_) & 0b1) << SPI_CFG1_RXDMAEN_Pos) |
                    ((static_cast<uint8_t>(enableTXdma_) & 0b1) << SPI_CFG1_TXDMAEN_Pos) |
                    ((static_cast<uint8_t>(crcSize_ - 1) & 0b11111) << SPI_CFG1_CRCSIZE_Pos) |
                    ((static_cast<uint8_t>(crcEnable_) & 0b1) << SPI_CFG1_CRCEN_Pos) |
                    ((static_cast<uint8_t>(masterDivider_) & 0b111) << SPI_CFG1_MBR_Pos) 
                );

                static_assert(!((ssIdleness_ || dataIdleness_) && protocol_ == protocol::ti), "The SS and DATA idleness is not supported in TI mode!");
                static_assert(ssIdleness_ < 16, "The SS idleness less than 16 clock periods!");
                static_assert(dataIdleness_ < 16, "The DATA idleness less than 16 clock periods!");

                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG2)>( 
                    ((static_cast<uint8_t>(ssIdleness_) & 0b1111) << SPI_CFG2_MSSI_Pos) |
                    ((static_cast<uint8_t>(dataIdleness_) & 0b1111) << SPI_CFG2_MIDI_Pos) |
                    ((static_cast<uint8_t>(ioSwap_) & 0b1) << SPI_CFG2_IOSWP_Pos) |
                    ((static_cast<uint8_t>(mode_) & 0b11) << SPI_CFG2_COMM_Pos) |
                    ((static_cast<uint8_t>(protocol_) & 0b111) << SPI_CFG2_SP_Pos) |
                    ((static_cast<uint8_t>(role_) & 0b1) << SPI_CFG2_MASTER_Pos) |
                    ((static_cast<uint8_t>(bitOrder_) & 0b1) << SPI_CFG2_LSBFRST_Pos) |
                    ((static_cast<uint8_t>(clockPhase_) & 0b1) << SPI_CFG2_CPHA_Pos) |
                    ((static_cast<uint8_t>(clockPol_) & 0b1) << SPI_CFG2_CPOL_Pos) |
                    ((static_cast<uint8_t>(ssOrigin_) & 0b1) << SPI_CFG2_SSM_Pos) |
                    ((static_cast<uint8_t>(ssPol_) & 0b1) << SPI_CFG2_SSIOP_Pos) |
                    ((static_cast<uint8_t>(ssOutputEnable_) & 0b1) << SPI_CFG2_SSOE_Pos) |
                    ((static_cast<uint8_t>(ssBehavior_) & 0b1) << SPI_CFG2_SSOM_Pos) |
                    ((static_cast<uint8_t>(afBehavior_) & 0b1) << SPI_CFG2_AFCNTR_Pos) 
                );
            }

            void enable(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_SPE_Pos);
            }

            void disable(){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_SPE_Pos);
            }

            void startTransfer(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_CSTART_Pos);
            }

            void requestSuspend(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_CSUSP_Pos);
            }

            void setDirection(direction dir_){
                if(dir_ == direction::receiver){
                    reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_HDDIR_Pos);
                }else reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_HDDIR_Pos);
            }

            void setInternalSS(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_SSI_Pos);
            }

            void clearInternalSS(){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_SSI_Pos);
            }

            void enableCRC(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(0b1 << SPI_CFG1_CRCEN_Pos);
            }

            void disableCRC(){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(0b1 << SPI_CFG1_CRCEN_Pos);
            }

            void setCRC(uint32_t crc_){
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CRCPOLY)>(crc_);
            }

            void setTxCRC(uint32_t crc_){
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, TXCRC)>(crc_);
            }
            
            void setRxCRC(uint32_t crc_){
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, RXCRC)>(crc_);
            }

            void lockIO(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR1)>(0b1 << SPI_CR1_IOLOCK_Pos);
            }

            /// @brief Sets number of data for the current transfer. Can be changed only while the SPI peripheral is disabled.
            /// @param num_ Number of data to transfer
            void setNumberOfData(uint16_t num_){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR2)>(0xFFFF << SPI_CR2_TSIZE_Pos);
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR2)>(num_ << SPI_CR2_TSIZE_Pos);
            }

            /// @brief Sets number of data tranfer extension to be reloaded into the current transfer size after transaction. Can be set only if cleared by hardware.
            /// @param num_ Number of data to reload
            void setNumberOfDataExtension(uint16_t num_){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR2)>(0xFFFF << SPI_CR2_TSER_Pos);
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CR2)>(num_ << SPI_CR2_TSER_Pos);
            }

            void enableRxDMA(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(0b1 << SPI_CFG1_RXDMAEN_Pos);
            }

            void disableRxDMA(){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(0b1 << SPI_CFG1_RXDMAEN_Pos);
            }

            void enableTxDMA(){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(0b1 << SPI_CFG1_TXDMAEN_Pos);
            }

            void disableTxDMA(){
                reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(0b1 << SPI_CFG1_TXDMAEN_Pos);
            }

            void enableInterrupt(interrupt int_){
                //All these three interrupts are enabled by this bit
                if(int_ == interrupt::endoftransfer || int_ == interrupt::suspend || int_ == interrupt::txcomplete){
                    reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, IER)>(0b1 << SPI_IER_EOTIE_Pos);
                }else reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, IER)>(int_);
                
            }

            void disableInterrupt(interrupt int_){
                //All these three interrupts are enabled by this bit
                if(int_ == interrupt::endoftransfer || int_ == interrupt::suspend || int_ == interrupt::txcomplete){
                    reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, IER)>(0b1 << SPI_IER_EOTIE_Pos);
                }else reg::clear<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, IER)>(int_);
            }

            bool getInterruptFlag(interrupt int_){
                return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, SR)>(int_));
            }

            void clearInterruptFlag(interrupt int_){
                reg::set<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, CFG1)>(int_);
            }

            bool rxFifoNotEmpty(){
                return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, SR)>(0b1 << SPI_SR_RXWNE_Pos));
            }

            uint8_t rxFifoFramesAvailable(){
                return (reg::read<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, SR)>(0b11 << SPI_SR_RXPLVL_Pos) >> 13);
            }

            uint16_t remainingNumberOfData(){
                return (reg::read<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, SR)>(0xFFFF << SPI_SR_CTSIZE_Pos) >> 16);
            }

            void setUnderrunPattern(uint32_t pattern_){
                reg::write<static_cast<uint32_t>(peripheral_) + offsetof(SPI_TypeDef, UDRDR)>(pattern_);
            }

    };

}

#endif
