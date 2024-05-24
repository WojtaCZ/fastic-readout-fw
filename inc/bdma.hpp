#ifndef BDMA_H
#define BDMA_H

#include <cstdint>
#include <cstddef>
#include "register.hpp"
#include "stm32h753xx.h"

namespace bdma{
    enum class peripheral : uint32_t {
        dma1 = BDMA_BASE
    };

    enum class channel {
        channel0 = 0x008UL,
        channel1 = 0x01CUL,
        channel2 = 0x030UL,
        channel3 = 0x044UL,
        channel4 = 0x058UL,
        channel5 = 0x06CUL,
        channel6 = 0x080UL,
        channel7 = 0x094UL,
    };

    enum class priority{
        low = 0b00,
        medium = 0b01,
        high = 0b10,
        veryhigh = 0b11
    };

    enum class mode{
        periph2mem = 0b00,
        mem2periph = 0b01,
        mem2mem = 0b10,
    };

    enum class datasize{
        byte = 0b00,
        halfword = 0b01,
        word = 0b10
    };

    enum class pincoffset{
        psize = 0b0,
        word = 0b1,
    };

    enum class targetmem{
        mem0 = 0b0,
        mem1 = 0b1,
    };

    enum class interrupt : uint32_t{
        global                  = 0x0001, 
        transferComplete        = 0x0002,
        transferHalfComplete    = 0x0004,
        transferError           = 0x0008
    };


    template<peripheral peripheral_, channel channel_, mode mode_, datasize msize_, bool mincrement_, uint32_t m0address_, datasize psize_, bool pincrement_, uint32_t paddress_, uint16_t numofdata_,
             priority priority_ = priority::low, bool circular_ = false, pincoffset pincoffset_ = pincoffset::psize, bool doublebuffer_ = false, uint32_t m1address_ = 0>
    class bdma{
        public:
            bdma(){
                reg::write<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CR)>( 
                    //Interrupts are not enabled here and the channel is not yet being enabled
                    ((static_cast<uint8_t>(mode_) & 0b01) << BDMA_CCR_DIR_Pos) |
                    ((static_cast<uint8_t>(circular_) & 0b1) << BDMA_CCR_CIRC_Pos) |
                    ((static_cast<uint8_t>(pincrement_) & 0b1) << BDMA_CCR_PINC_Pos) |
                    ((static_cast<uint8_t>(mincrement_) & 0b1) << BDMA_CCR_MINC_Pos) |
                    ((static_cast<uint8_t>(psize_) & 0b11) << BDMA_CCR_PSIZE_Pos) | 
                    ((static_cast<uint8_t>(msize_) & 0b11) << BDMA_CCR_MSIZE_Pos) | 
                    ((static_cast<uint8_t>(priority_) & 0b11) << BDMA_CCR_PL_Pos) | 
                    ((static_cast<uint8_t>(mode_) & 0b10) << BDMA_CCR_MEM2MEM_Pos - 1) | 
                    ((static_cast<uint8_t>(doublebuffer_) & 0b1) << BDMA_CCR_DBM_Pos) |
                    //Current target is not set here
                );

                reg::write<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CPAR)>(paddress_);
                reg::write<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CM0AR)>(m0address_);
                reg::write<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_)  + offsetof(BDMA_Channel_TypeDef, CM1AR)>(m1address_);
                reg::write<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) +  offsetof(BDMA_Channel_TypeDef, CNDTR)>(numofdata_);
            }

            void enable(){
                reg::set<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_EN_Pos);
            }

            void disable(){
                reg::clear<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_EN_Pos);
            }

            void setTargetMemory(targetmem mem_){
                reg::set<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>((mem_ & 0b1) << BDMA_CCR_CT_Pos);
            }

            void enableInterruptFlag(std::vector<interrupt> int_){

                static_assert(int_ != interrupt::global, "Global interrupt cannot be enabled!");

                uint8_t mask_ = 0;

                for(auto i : int_){
                    //Merge the interrupts into single mask
                    switch(int_){
                    case interrupt::transferHalfComplete:
                        mask_ |= (0b1 << BDMA_CCR_HTIE_Pos);
                        break;
                    case interrupt::transferComplete:
                        mask_ |= (0b1 << BDMA_CCR_TCIE_Pos);
                        break;
                    case interrupt::transferError:
                        mask_ |= (0b1 << BDMA_CCR_TEIE_Pos);

                        
                    default:
                        break;
                    }
                    
                }
                
                //Apply the mask to the control register
                reg::set<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(mask_);
            }

            void enableInterruptFlag(interrupt int_){

                static_assert(int_ != interrupt::global, "Global interrupt cannot be enabled!");

                switch(int_){
                case interrupt::transferHalfComplete:
                    reg::set<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_HTIE_Pos);
                    break;
                case interrupt::transferComplete:
                    reg::set<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_TCIE_Pos);
                    break;
                case interrupt::transferError:
                    reg::set<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_TEIE_Pos); 
                default:
                    break;
                }
            }

            void disableInterruptFlag(std::vector<interrupt> int_){

                static_assert(int_ != interrupt::global, "Global interrupt cannot be disabled!");

                uint8_t mask_ = 0;

                for(auto i : int_){
                    //Merge the interrupts into single mask
                    switch(int_){
                    case interrupt::transferHalfComplete:
                        mask_ |= (0b1 << BDMA_CCR_HTIE_Pos);
                        break;
                    case interrupt::transferComplete:
                        mask_ |= (0b1 << BDMA_CCR_TCIE_Pos);
                        break;
                    case interrupt::transferError:
                        mask_ |= (0b1 << BDMA_CCR_TEIE_Pos);

                    default:
                        break;
                    }
                    
                }
                
                //Apply the mask to the control register
                reg::clear<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(mask_);
            }

            void disableInterruptFlag(interrupt int_){

                static_assert(int_ != interrupt::global, "Global interrupt cannot be disabled!");

                switch(int_){
                case interrupt::transferHalfComplete:
                    reg::clear<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_HTIE_Pos);
                    break;
                case interrupt::transferComplete:
                    reg::clear<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_TCIE_Pos);
                    break;
                case interrupt::transferError:
                    reg::clear<static_cast<uint32_t>(peripheral_) + static_cast<uint32_t>(channel_) + offsetof(BDMA_Channel_TypeDef, CCR)>(0b1 << BDMA_CCR_TEIE_Pos); 
                default:
                    break;
                }
            }

            void clearInterruptFlag(interrupt int_){
                switch (channel_){
                    case channel::channel0:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_);
                    case channel::channel1:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 4);
                    case channel::channel2:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 8);
                    case channel::channel3:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 12);
                    case channel::channel4:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 16);
                    case channel::channel5:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 20);
                    case channel::channel6:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 24);
                    case channel::channel7:
                        reg::write<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, IFCR)>(int_ << 28);
                    
                    default:
                        break;
                }
            }

            bool getInterruptFlag(){
                switch (channel_){
                    case channel::channel0:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_));
                    case channel::channel1:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 4));
                    case channel::channel2:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 8));
                    case channel::channel3:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 12));
                    case channel::channel4:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 16));
                    case channel::channel5:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 20));
                    case channel::channel6:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 24));
                    case channel::channel7:
                        return static_cast<bool>(reg::read<static_cast<uint32_t>(peripheral_) + offsetof(BDMA_TypeDef, ISR)>(int_ << 28));
                    
                    default:
                        break;
                }
            }

    };

}



#endif
