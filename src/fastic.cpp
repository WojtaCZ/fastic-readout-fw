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

__attribute__((section(".dma_buffer"))) uint8_t fastic1_buffers[2][1024];

uint32_t stat = 0;
namespace fastic {
    using namespace stmcpp::units;
    static constexpr uint16_t fasticBufferSize = 1024;

    uint32_t tmpReg, slip, valid, errors;
    uint64_t data;

     // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    //__attribute__((section(".dma_buffer"))) uint16_t fastic1_buffers[2][1024];
    __attribute__((section(".dma_buffer"))) uint32_t fastic2_buffers[2][1024];


    
    aurora::rx fastic1(&fastic2_buffers[0][0], 1000);
    aurora::rx fastic2(&fastic2_buffers[0][0], 1000);

    /**;
     * 
     * FASTIC1 GPIO CONFIGURATION
     * 
     */
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 11>  fastic1_rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 12>  fastic1_rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    
    // I2C3 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 7> fastic1_scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 8> fastic1_sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    
    // SPI2 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 3>  fastic1_mosi   (stmcpp::gpio::mode::af5);
    stmcpp::gpio::pin<stmcpp::gpio::port::porti, 1>  fastic1_clk    (stmcpp::gpio::mode::af5);
    
    // ADC3_INP13
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 2>  fastic1_vmon (stmcpp::gpio::mode::analog);
    
    // TIME input
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 15>  fastic1_time (stmcpp::gpio::mode::input);
    
    // Injection output
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 14>  fastic1_inj (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);
    
    /**
     * 
     * FASTIC1 PERIPHERAL INITIALIZATION
     * 
     */

    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c3> fastic1_i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    stmcpp::i2c::address fastic1_address (0x10);

    // Set up the SPI
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi2> fastic1_spi (stmcpp::spi::role::slave, stmcpp::spi::mode::rxSimplex, 8, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::msbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
    
    // Set up the DMA
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel1> dmamux1ch1(stmcpp::dmamux1::request::spi2_rx_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream1> fastic1_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::byte, false, static_cast<uint32_t>(SPI2_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::byte, true, (uint32_t)&fastic1_buffers[0][0], (uint32_t)&fastic1_buffers[1][0], fasticBufferSize, stmcpp::dma::priority::veryHigh, false, stmcpp::dma::pincOffset::psize, true);

    /**
     * 
     * FASTIC2 GPIO CONFIGURATION
     * 
     */
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 4>  fastic2_rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 5>  fastic2_rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    
    // I2C4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 12> fastic2_scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 13> fastic2_sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    
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

        fastic1_rst_n.set();
        fastic2_rst_n.set();

        stmcpp::clock::systick::waitBlocking(10_ms);

        fastic1_i2c.enable();
        fastic2_i2c.enable();

        // Read out the ASIC version to check communication
        //tmpReg = fastic1_i2c.readRegister(0x7f,fastic1_address);

        // Read out the ASIC version to check communication
        tmpReg = fastic1_i2c.readRegister(0x7f, fastic1_address);
        tmpReg = fastic2_i2c.readRegister(0x7f, fastic2_address);

        // Reduce the serializer speed to the minimum
        static constexpr uint8_t speed = 0x04;
        static constexpr uint8_t spReg = 0x02 | (speed << 3); 
        fastic1_i2c.writeRegister(0xb9, spReg , fastic1_address);
        fastic2_i2c.writeRegister(0xb9, spReg , fastic2_address);

      
        /*fastic1_i2c.writeRegister(0xA3, 0xAB, fastic2_address);
        fastic1_i2c.writeRegister(0xA4, 0xAB, fastic2_address);
        fastic1_i2c.writeRegister(0xA5, 0xAB, fastic2_address);
        fastic1_i2c.writeRegister(0xA6, 0xAB, fastic2_address);
        // Force contnuous serializer word
        fastic1_i2c.writeRegister(0x87, (0x3F | 0x80) , fastic2_address);*/
        
        // Disable scrambling on the aurora bus
        fastic1_i2c.writeRegister(0x89, 0x00, fastic1_address);


        //syncClock();
        
        fastic1_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        fastic2_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream1_IRQn);
        NVIC_EnableIRQ(DMA1_Stream0_IRQn);
        fastic1_dma.setNumberOfData(fasticBufferSize);
        fastic2_dma.setNumberOfData(2*fasticBufferSize);
        fastic1_dma.disableDoubleBuffer();
        fastic2_dma.disableDoubleBuffer();

        fastic1_spi.setNumberOfData(fasticBufferSize);
        fastic2_spi.setNumberOfData(2*fasticBufferSize);

        fastic1_spi.enableSoftwareSS();
        fastic2_spi.enableSoftwareSS();
        fastic1_spi.enableRxDma();
        fastic2_spi.enableRxDma();

        fastic1_dma.enable();
        fastic2_dma.enable();
        fastic1_spi.enable();
        //fastic2_spi.enable();

    }

    void syncClock() {

      /*  fastic2_spi.enableInterrupt(stmcpp::spi::interrupt::overrun);
        fastic2_spi.enableInterrupt(stmcpp::spi::interrupt::modeFault);
        fastic2_spi.enableInterrupt(stmcpp::spi::interrupt::duplexPacket);*/

        // Force the FastIC+ to output a known serializer word (a square wave in this case)
        // Serializer word (MSB to LSB)
        fastic2_i2c.writeRegister(0xA3, 0x55, fastic2_address);
        fastic2_i2c.writeRegister(0xA4, 0x55, fastic2_address);
        fastic2_i2c.writeRegister(0xA5, 0x55, fastic2_address);
        fastic2_i2c.writeRegister(0xA6, 0x55, fastic2_address);
        // Force contnuous serializer word
        fastic2_i2c.writeRegister(0x87, (0x3F | 0x80) , fastic2_address);

 
       
        /*ad9510::setChannelDelayCoarse(ad9510::channel::out6, ad9510::rampCaps::c4, ad9510::rampCurrent::u200);
        ad9510::setChannelDelayFine(ad9510::channel::out6, 10);
        ad9510::enableChannelDelay(ad9510::channel::out6);
        stmcpp::clock::systick::waitBlocking(100_ms);*/

        /*fastic2_spi.disable();
        fastic2_dma.disable();

        fastic2_dma.disableDoubleBuffer();
        fastic2_dma.setNumberOfData(1);

        fastic2_spi.setNumberOfData(0);
        fastic2_spi.enableIoSwap();
        fastic2_spi.enableSoftwareSS();
        fastic2_spi.enableRxDma();

        fastic2_dma.enable();
        fastic2_spi.enable();



        //fastic2_spi.setInternalSs();

        while(true){
                if(fastic2_buffers[0][0] == 0x12345678){
                    __ASM volatile("bkpt");
                    break;
                } else {
                    fastic2_spi.disable();
                    fastic2_spi.enable();
                }
        }

        

        fastic2_dma.disable();
        fastic2_dma.enableDoubleBuffer();
        fastic2_dma.setNumberOfData(fasticBufferSize);
        fastic2_dma.enable();
        */
    }
}

extern "C" void DMA_STR1_IRQHandler(){
    fastic::fastic1_dma.disable();
   //__ASM volatile("bkpt");
    //fastic::fastic1_dma.disableInterrupt(stmcpp::dma::interrupt::transferComplete);
    fastic::fastic1_dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
    NVIC_DisableIRQ(DMA1_Stream1_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
  /*if(!fastic::fastic1.synchronize()){
		__ASM volatile("bkpt");
	}else{
        fastic::slip = fastic::fastic1.getBitSlip();
	}*/

}

extern "C" void DMA_STR0_IRQHandler(){
    fastic::fastic2_dma.disable();
   __ASM volatile("bkpt");

   if(!fastic::fastic2.synchronize()){
		__ASM volatile("bkpt");
	}else{
        fastic::slip = fastic::fastic2.getBitSlip();
	}
        
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
