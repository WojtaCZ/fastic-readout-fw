#include "stm32h753xx.h"
#include "fastic.hpp"
#include "ad9510.hpp"
#include "power.hpp"

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


uint32_t cntr = 0;

uint8_t tbuf[1024];

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

        
        // Deassert reset pin
        rst_n.set();
        stmcpp::clock::systick::waitBlocking(1_ms);
        
        // Enable I2C
        i2c.enable();
 
        // Read out the ASIC version to check communication
        if(i2c.readRegister(0x7f, address) == 0x00) return false;

        // Reduce the serializer clock speed to 40MHz (80Mbps)
        i2c.writeRegister(0xb9,  (0x02 | (0x04 << 3)) , address);
        
        // Configure the DMA
        dma.setNumberOfData(4*bufferSize);
        dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream1_IRQn);

        // Configure SPI
        spi.setNumberOfData(4*bufferSize);
        spi.enableSoftwareSS();
        spi.enableRxDma();
        
        // Enable the DMA (SPI is enabled separately by a command)
        dma.enable();

        forceWordMode(0x12345678);
          // Disable scrambling on the aurora bus
        fastic1::i2c.writeRegister(0x89, 0x00, fastic1::address);

        return true;
        //fastic_spi.enable();
 
    }

    void enableStream(){
        spi.enable();
    }

    void disableStream(){
        spi.disable();
    }

    void forceWordMode(uint32_t word) {
        fastic1::i2c.writeRegister(0xA6, word & 0xFF, fastic1::address);
        fastic1::i2c.writeRegister(0xA5, (word >> 8) & 0xFF, fastic1::address);
        fastic1::i2c.writeRegister(0xA4, (word >> 16) & 0xFF, fastic1::address);
        fastic1::i2c.writeRegister(0xA3, (word >> 24) & 0xFF, fastic1::address);

        // Force contnuous serializer word
        fastic1::i2c.writeRegister(0x87, (0x3F | 0x80) , fastic1::address);
        
        // Disable scrambling on the aurora bus
        fastic1::i2c.writeRegister(0x89, 0x00, fastic1::address);
    }

    extern "C" void DMA_STR1_IRQHandler(){
        fastic1::dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
        NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
        //dma.disable();
        /*cntr++;
        fastic1::buffers[((~DMA1_Stream1->CR) & DMA_SxCR_CT_Msk) >> 19][0] = cntr;*/
        tud_vendor_n_write(0, (uint8_t *)fastic1::buffers[((~DMA1_Stream1->CR) & DMA_SxCR_CT_Msk) >> 19], fastic1::bufferSize*4);
        //dma.enable();
        //tud_vendor_n_write_flush(0);


        //printf("%d %01d\n\r", stmcpp::clock::systick::getTicks(), ((~DMA1_Stream1->CR) & DMA_SxCR_CT_Msk) >> 19);
        

    }
}

namespace fastic {
    using namespace stmcpp::units;
    static constexpr uint16_t fasticBufferSize = 1024;

    uint32_t tmpReg, slip, valid, errors;
    uint64_t data;

     // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    //__attribute__((section(".dma_buffer"))) uint16_t fastic1_buffers[2][1024];
    __attribute__((section(".dma_buffer"))) uint32_t fastic2_buffers[2][1024];


    


    /**;
     * 
     * FASTIC1 GPIO CONFIGURATION
     * 
     */
    
    
    /**
     * 
     * FASTIC1 PERIPHERAL INITIALIZATION
     * 
     */

 
    /**
     * 
     * FASTIC2 GPIO CONFIGURATION
     * 
     */
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 4>  fastic2_rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 5>  fastic2_rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    
    // I2C4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 12> fastic2_scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::medium);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 13> fastic2_sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::medium);
    
    // SPI4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 14>  fastic2_mosi   (stmcpp::gpio::mode::af5);
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 12>  fastic2_clk    (stmcpp::gpio::mode::af5);
    
    // ADC2_INP14
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 2>  fastic2_vmon (stmcpp::gpio::mode::analog);
    
    // TIME input
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 6>  fastic2_time (stmcpp::gpio::mode::input);
    
    // Injection output
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 11>  fastic2_inj (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);
    
    /**
     * 
     * FASTIC2 PERIPHERAL INITIALIZATION
     * 
     */

    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c4> fastic2_i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    stmcpp::i2c::address fastic2_address (0x10);

    // Set up the SPI
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi4> fastic2_spi (stmcpp::spi::role::slave, stmcpp::spi::mode::rxSimplex, 16, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::lsbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
    
    // Set up the DMA
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel0> dmamux1ch0(stmcpp::dmamux1::request::spi4_rx_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream0> fastic2_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(SPI4_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&fastic2_buffers[0][0], (uint32_t)&fastic2_buffers[1][0], fasticBufferSize, stmcpp::dma::priority::veryHigh, false, stmcpp::dma::pincOffset::psize, true);


    void initInjectionChannels() {

           

        // Timer 15 is used as a trigger (generate a 500kHz base clock) 
        stmcpp::reg::write(std::ref(TIM15->PSC), 239);   
        stmcpp::reg::write(std::ref(TIM15->ARR), 499000);
        // Load all the registers
        stmcpp::reg::set(std::ref(TIM15->EGR), TIM_EGR_UG | TIM_EGR_TG);
        //Enable the timer and its output
        stmcpp::reg::set(std::ref(TIM15->CR1), TIM_CR1_CEN);



        // Timer 1 is used to inject pulses into the fastic injection channels

        // Slave mode: combined reset & trigger
        stmcpp::reg::write(std::ref(TIM1->SMCR), (0b1000 << TIM_SMCR_SMS_Pos));

        // Enable CH2_P and CH3_P outputs and configure them to be active low
	    stmcpp::reg::set(std::ref(TIM1->CCER), TIM_CCER_CC2E | /*TIM_CCER_CC2P |*/ TIM_CCER_CC3E /*| TIM_CCER_CC3P*/ );

        // Select the output compare mode 
	    stmcpp::reg::set(std::ref(TIM1->CCMR1), TIM_CCMR1_OC2M);
	    stmcpp::reg::set(std::ref(TIM1->CCMR2), TIM_CCMR2_OC3M);
        
        stmcpp::reg::write(std::ref(TIM1->CCR2), 23);
        stmcpp::reg::write(std::ref(TIM1->CCR3), 23);

        //stmcpp::reg::write(std::ref(TIM1->PSC), 23);   
	
        // Load all the registers
        stmcpp::reg::set(std::ref(TIM1->EGR), TIM_EGR_UG);
        //Enable the timer and its output
        stmcpp::reg::set(std::ref(TIM1->CR1), TIM_CR1_CEN);
        stmcpp::reg::set(std::ref(TIM1->BDTR), TIM_BDTR_MOE);

    }
       
    void init() {


       // printf("Enabling 1V2\n\r");
        power::enable1V2();
        stmcpp::clock::systick::waitBlocking(1_ms);

        /*power::isPowerGood(power::ldo::D1V2);
        s = power::isPowerGood(power::ldo::T1V2);
        s = power::isPowerGood(power::ldo::A1V2);*/

        fastic2_rst_n.set();

        stmcpp::clock::systick::waitBlocking(10_ms);

        fastic2_i2c.enable();

        // Read out the ASIC version to check communication
        //tmpReg = fastic1_i2c.readRegister(0x7f,fastic1_address);

        // Read out the ASIC version to check communication
        tmpReg = fastic2_i2c.readRegister(0x7f, fastic2_address);

        // Reduce the serializer speed to the minimum
        static constexpr uint8_t speed = 0x04;
        static constexpr uint8_t spReg = 0x02 | (speed << 3); 
        fastic2_i2c.writeRegister(0xb9, spReg , fastic2_address);

      
        /*fastic1_i2c.writeRegister(0xA3, 0xAB, fastic2_address);
        fastic1_i2c.writeRegister(0xA4, 0xAB, fastic2_address);
        fastic1_i2c.writeRegister(0xA5, 0xAB, fastic2_address);
        fastic1_i2c.writeRegister(0xA6, 0xAB, fastic2_address);
        // Force contnuous serializer word
        fastic1_i2c.writeRegister(0x87, (0x3F | 0x80) , fastic2_address);*/
        
        // Disable scrambling on the aurora bus


        //syncClock();
        
        fastic2_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream1_IRQn);
        NVIC_EnableIRQ(DMA1_Stream0_IRQn);

        fastic2_dma.setNumberOfData(2*fasticBufferSize);

        fastic2_dma.disableDoubleBuffer();


        fastic2_spi.setNumberOfData(2*fasticBufferSize);


        fastic2_spi.enableSoftwareSS();

        fastic2_spi.enableRxDma();


        fastic2_dma.enable();

        //fastic2_spi.enable();

    }


    // FastIC reset pins are 1V8 logic - this voltage is not sufficient to drive the 3V3 input of the STM to high level, thus it would always report a low level when reading the port
    // The best we can do here is report the state of the ODR, though it is not the actual state of the pin
    
    bool getFastICSyncReset(identifier id) {
        if(id == identifier::FASTIC1) {
            return fastic1::rstcnt_n.getIntendedState();
        } else {
            return fastic2_rstcnt_n.getIntendedState();
        }
    }


    void setFastICSyncReset(identifier id, uint8_t value) {
        if(value) {
            if(id == identifier::FASTIC1) {
                fastic1::rstcnt_n.set();
            } else {
                fastic2_rstcnt_n.set();
            }
        } else {
            if(id == identifier::FASTIC1) {
                fastic1::rstcnt_n.clear();
            } else {
                fastic2_rstcnt_n.clear();
            }
        }
    }

    bool getFastICTime(identifier id) {
        if(id == identifier::FASTIC1) {
            return fastic1::time.read();
        } else {
            return fastic2_time.read();
        }
    }

    uint8_t getFastICRegister(identifier id, uint8_t address) {
        if(id == identifier::FASTIC1) {
            return fastic1::i2c.readRegister(address, fastic1::address);
        } else {
            return fastic2_i2c.readRegister(address, fastic2_address);
        }
    }

    bool setFastICRegister(identifier id, uint8_t address, uint8_t value) {
        if(id == identifier::FASTIC1) {
            fastic1::i2c.writeRegister(address, value, fastic1::address);
        } else {
            fastic2_i2c.writeRegister(address, value, fastic2_address);
        }

        return true;
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



extern "C" void DMA_STR0_IRQHandler(){
   /* fastic::fastic2_dma.disable();
   __ASM volatile("bkpt");

   if(!fastic::fastic2.synchronize()){
		__ASM volatile("bkpt");
	}else{
        fastic::slip = fastic::fastic2.getBitSlip();
	}*/
        
    /*fastic::fastic2.processBuffer();
    auto packets = fastic::fastic2.getPacketBuffer();
    fastic::valid = packets.size();
    fastic::errors = std::count_if(packets.begin(), packets.end(), [](aurora::packet packet) { return (packet.getType() == aurora::packet::type::error); });
    fastic::slip = fastic::fastic2.getBitSlip();

    for (aurora::packet p : packets){
        fastic::data = p.getData();
        __ASM volatile("bkpt");
    }*/

   __ASM volatile("bkpt");
}
