#include "stm32h753xx.h"
#include "fastic.hpp"
#include "ad9510.hpp"
#include "power.hpp"
#include "board.hpp"

#include <aurora.hpp>

#include <stmcpp/register.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/spi.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/dmamux.hpp>
#include <stmcpp/i2c.hpp>

#include <tinyusb/src/class/vendor/vendor_device.h>
#include <tinyusb/src/device/usbd.h>
#include <tinyusb/src/class/cdc/cdc_device.h>


namespace fastic::common {
    void init() {
        // Enable fastic power
        power::enable1V2();

        // Timer 4 is used as a trigger (generate a pulse with frequency of 50us base clock) 
        stmcpp::reg::write(std::ref(TIM4->PSC), 24 - 1);   
        stmcpp::reg::write(std::ref(TIM4->ARR), 500-1);

        // Load all the registers
        stmcpp::reg::set(std::ref(TIM4->EGR), TIM_EGR_UG | TIM_EGR_TG);
        // Enable the timer and configure its output trigger (TRGO)
        stmcpp::reg::write(std::ref(TIM4->CR2), (0b010 << 4)); // Set TRGO to update event
        stmcpp::reg::set(std::ref(TIM4->CR1), TIM_CR1_CEN);
        // Timer 1 is used to inject pulses into the fastic injection channels
        // Slave mode: combined reset & trigger
        // Set Timer 4 as the trigger source (ITR3 for TIM1)
        //stmcpp::reg::write(std::ref(TIM1->SMCR), (0b1 << 16) | (0b11 << 4) | (0b0011));
        stmcpp::reg::write(std::ref(TIM1->CR1), 0b1 << TIM_CR1_OPM_Pos);
        stmcpp::reg::write(std::ref(TIM1->SMCR), (0b1 << 16) | (0b11 << 4));

        stmcpp::reg::write(std::ref(TIM1->PSC), 0);
        stmcpp::reg::write(std::ref(TIM1->ARR), 48 - 1);

        // Select the output compare mode 
	    stmcpp::reg::set(std::ref(TIM1->CCMR1), (0b1 << 24));
	    stmcpp::reg::set(std::ref(TIM1->CCMR2), (0b1 << 24));
        
        // Set the pulse width - 100ns
        stmcpp::reg::write(std::ref(TIM1->CCR2), 0);
        stmcpp::reg::write(std::ref(TIM1->CCR4), 0);
        

        // Load all the registers
        stmcpp::reg::set(std::ref(TIM1->EGR), TIM_EGR_UG);
        //Enable the timer and its output
        stmcpp::reg::set(std::ref(TIM1->CR1), TIM_CR1_CEN);
        stmcpp::reg::set(std::ref(TIM1->BDTR), TIM_BDTR_MOE);

    }

    // Registers which could affect the functionality of the readout 
    bool registerRequireForce(uint8_t address) {
        if((address & 0xF0) == 0x50 || address == 0x66 || address >= 0x92) {
            return true;
        } else return false;
    }

    bool registerIsInRange(uint8_t address) {
        if(address <= 0xBD) {
            return true;
        } else return false;
    }
        
}


namespace fastic1{
    using namespace stmcpp::units;

    static constexpr uint32_t bufferSize = 1024;
    // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    __attribute__((section(".dma_buffer"))) uint32_t buffers[2][bufferSize];

    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 11>  rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 12>  rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    
    // I2C3 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 7> scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 8> sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    
    // SPI2 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 3>  mosi   (stmcpp::gpio::mode::af5);
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 1>  clk    (stmcpp::gpio::mode::af5);
    
    // ADC3_INP13
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 2>  vmon (stmcpp::gpio::mode::analog);
    
    // TIME input
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 15>  time (stmcpp::gpio::mode::input);
    
    // Injection output
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 14>  inj (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);

    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c3> i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    stmcpp::i2c::address address (0x10);
   
    // Set up the SPI;
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi2> spi (stmcpp::spi::role::slave, stmcpp::spi::mode::rxSimplex, 8, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::msbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
       
    // Set up the DMA
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel1> dmamux1ch1(stmcpp::dmamux1::request::spi2_rx_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream1> dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::byte, false, static_cast<uint32_t>(SPI2_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&buffers[0][0], (uint32_t)&buffers[1][0], 4*bufferSize, stmcpp::dma::priority::veryHigh, true, stmcpp::dma::pincOffset::psize, true);
    
    bool init() {

        // Check the power domains
        if(!power::isPowerGood(power::ldo::D1V2)) return false;
        if(!power::isPowerGood(power::ldo::T1V2)) return false;
        if(!power::isPowerGood(power::ldo::A1V2)) return false;
        
        rst_n.clear();
        rstcnt_n.set();
        stmcpp::clock::systick::waitBlocking(1_ms);
        // Deassert reset pin
        rstcnt_n.clear();
        rst_n.set();
        stmcpp::clock::systick::waitBlocking(1_ms);
        
        // Enable I2C
        i2c.enable();
        
        uint8_t reg;

        // Read out the ASIC version to check communication
        if(i2c.readRegister(0x7f, address) == 0x00) return false;

        // Reduce the serializer clock speed to 40MHz (80Mbps)
        reg = i2c.readRegister(0xb9, address);
        i2c.writeRegister(0xb9, (reg | 0x20) , address);
        
        // Configure the DMA
        dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream1_IRQn);

        // Configure SPI
        spi.enableSoftwareSS();
        spi.enableRxDma();

        return true;
 
    }

    void enableStream(){
        tud_vendor_n_flush(0);
        dma.setNumberOfData(4*bufferSize);
        dma.enable();
        spi.enable();
    }

    void disableStream(){
        spi.disable();
        dma.disable();
        dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
        tud_vendor_n_flush(0);
    }

    bool isStreaming() {
        return spi.isEnabled();
    }

    void enableForceWordMode(uint32_t word) {
        i2c.writeRegister(0xA6, word & 0xFF, address);
        i2c.writeRegister(0xA5, (word >> 8) & 0xFF, address);
        i2c.writeRegister(0xA4, (word >> 16) & 0xFF, address);
        i2c.writeRegister(0xA3, (word >> 24) & 0xFF, address);

        uint8_t reg;
        // Force continuous serializer word
        reg = i2c.readRegister(0x87, address);
        i2c.writeRegister(0x87, (reg | 0x80) , address);
        
        // Disable scrambling on the aurora bus
        reg = i2c.readRegister(0x89, address);
        i2c.writeRegister(0x89, (reg & (~0x80)), address);
    }

    void disableForceWordMode() {
        uint8_t reg;
        // Disable Force continuous serializer word
        reg = i2c.readRegister(0x87, address);
        i2c.writeRegister(0x87, (reg & (~0x80)) , address);
        
        // Enable scrambling on the aurora bus
        reg = i2c.readRegister(0x89, address);
        i2c.writeRegister(0x89, (reg | 0x80), address);
    }

    void enableInjection() {
        // Enable CH2_N output
	    stmcpp::reg::set(std::ref(TIM1->CCER), TIM_CCER_CC2NE);
    }

    void disableInjection() {
        // Disable CH2_N output
        stmcpp::reg::clear(std::ref(TIM1->CCER), TIM_CCER_CC2NE);
    }

    bool isInjecting() {
        return static_cast<bool>(stmcpp::reg::read(std::ref(TIM1->CCER), TIM_CCER_CC2NE_Msk));
    }

    // FastIC reset pins are 1V8 logic - this voltage is not sufficient to drive the 3V3 input of the STM to high level, thus it would always report a low level when reading the port
    // The best we can do here is report the state of the ODR, though it is not the actual state of the pin
    bool getSyncReset() {
        return rstcnt_n.getIntendedState();
    }

    void setSyncReset(bool value) {
        if(value) {
            rstcnt_n.set();
        } else {
            rstcnt_n.clear();
        }
    }

    bool getTime() {
        return time.read();
    }

    uint8_t getRegister(uint8_t regAddress) {
        return i2c.readRegister(regAddress, address);
    }

    bool setRegister(uint8_t regAddress, uint8_t value) {
        i2c.writeRegister(regAddress, value, address);
        return true;
    }

    extern "C" void DMA_STR1_IRQHandler(){
       
        tud_vendor_n_write(0, (uint8_t *)buffers[((~DMA1_Stream1->CR) & DMA_SxCR_CT_Msk) >> 19], bufferSize*4);
        dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
        NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
    }
}

namespace fastic2 {
    using namespace stmcpp::units;

    static constexpr uint32_t bufferSize = 1024;
    // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    __attribute__((section(".dma_buffer"))) uint32_t buffers[2][bufferSize];

    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 4>  rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 5>  rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    
    // I2C4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 12> scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::medium);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 13> sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::medium);
    
    // SPI4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 14>  mosi   (stmcpp::gpio::mode::af5);
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 12>  clk    (stmcpp::gpio::mode::af5);
    
    // ADC2_INP14
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 2>  vmon (stmcpp::gpio::mode::analog);
    
    // TIME input
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 6>  time (stmcpp::gpio::mode::input);
    
    // Injection output
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 11>  inj (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);

    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c4> i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    stmcpp::i2c::address address (0x10);

    // Set up the SPI
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi4> spi (stmcpp::spi::role::slave, stmcpp::spi::mode::rxSimplex, 8, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::msbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
    
    // Set up the DMA
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel0> dmamux1ch0(stmcpp::dmamux1::request::spi4_rx_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream0> dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::byte, false, static_cast<uint32_t>(SPI4_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&buffers[0][0], (uint32_t)&buffers[1][0], 4*bufferSize, stmcpp::dma::priority::veryHigh, true, stmcpp::dma::pincOffset::psize, true);

    bool init() {

        // Check the power domains
        if(!power::isPowerGood(power::ldo::D1V2)) return false;
        if(!power::isPowerGood(power::ldo::T1V2)) return false;
        if(!power::isPowerGood(power::ldo::A1V2)) return false;
        

        rst_n.clear();
        rstcnt_n.set();
        stmcpp::clock::systick::waitBlocking(1_ms);
        // Deassert reset pin
        rstcnt_n.clear();
        rst_n.set();
        stmcpp::clock::systick::waitBlocking(1_ms);
        
        // Enable I2C
        i2c.enable();
        
        uint8_t reg;

        // Read out the ASIC version to check communication
        if(i2c.readRegister(0x7f, address) == 0x00) return false;

        // Reduce the serializer clock speed to 40MHz (80Mbps)
        reg = i2c.readRegister(0xb9, address);
        i2c.writeRegister(0xb9, (reg | 0x20) , address);
        
        // Configure the DMA
        dma.setNumberOfData(4*bufferSize);
        dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream0_IRQn);

        // Configure SPI
        spi.enableSoftwareSS();
        spi.enableRxDma();
        
        // Enable the DMA (SPI is enabled separately by a command)
        dma.enable();

        return true;
 
    }

    void enableStream(){
        tud_vendor_n_flush(1);
        dma.setNumberOfData(4*bufferSize);
        dma.enable();
        spi.enable();
    }

    void disableStream(){
        spi.disable();
        dma.disable();
        dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
        tud_vendor_n_flush(1);
    }

    bool isStreaming() {
        return spi.isEnabled();
    }

    void enableForceWordMode(uint32_t word) {
        i2c.writeRegister(0xA6, word & 0xFF, address);
        i2c.writeRegister(0xA5, (word >> 8) & 0xFF, address);
        i2c.writeRegister(0xA4, (word >> 16) & 0xFF, address);
        i2c.writeRegister(0xA3, (word >> 24) & 0xFF, address);

        uint8_t reg;
        // Force continuous serializer word
        reg = i2c.readRegister(0x87, address);
        i2c.writeRegister(0x87, (reg | 0x80) , address);
        
        // Disable scrambling on the aurora bus
        reg = i2c.readRegister(0x89, address);
        i2c.writeRegister(0x89, (reg & (~0x80)), address);
    }

    void disableForceWordMode() {
        uint8_t reg;
        // Disable Force continuous serializer word
        reg = i2c.readRegister(0x87, address);
        i2c.writeRegister(0x87, (reg & (~0x80)) , address);
        
        // Enable scrambling on the aurora bus
        reg = i2c.readRegister(0x89, address);
        i2c.writeRegister(0x89, (reg | 0x80), address);
    }

    void enableInjection() {
        // Enable CH2_N output
	    stmcpp::reg::set(std::ref(TIM1->CCER), TIM_CCER_CC4E);
    }

    void disableInjection() {
        // Disable CH2_N output
        stmcpp::reg::clear(std::ref(TIM1->CCER), TIM_CCER_CC4E);
    }

    bool isInjecting() {
        return static_cast<bool>(stmcpp::reg::read(std::ref(TIM1->CCER), TIM_CCER_CC4E_Msk));
    }

    // FastIC reset pins are 1V8 logic - this voltage is not sufficient to drive the 3V3 input of the STM to high level, thus it would always report a low level when reading the port
    // The best we can do here is report the state of the ODR, though it is not the actual state of the pin
    bool getSyncReset() {
        return rstcnt_n.getIntendedState();
    }

    void setSyncReset(bool value) {
        if(value) {
            rstcnt_n.set();
        } else {
            rstcnt_n.clear();
        }
    }

    bool getTime() {
        return time.read();
    }

    uint8_t getRegister(uint8_t regAddress) {
        return i2c.readRegister(regAddress, address);
    }

    bool setRegister(uint8_t regAddress, uint8_t value) {
        i2c.writeRegister(regAddress, value, address);
        return true;
    }

    extern "C" void DMA_STR0_IRQHandler(){

        tud_vendor_n_write(1, (uint8_t *)buffers[((~DMA1_Stream0->CR) & DMA_SxCR_CT_Msk) >> 19], bufferSize*4);
        dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
        NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);

    }
     

}



