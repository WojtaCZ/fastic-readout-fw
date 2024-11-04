#include "stm32h753xx.h"
#include "fastic.hpp"
#include "ad9510.hpp"

#include <aurora.hpp>

#include <stmcpp/register.hpp>
#include <stmcpp/units.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/spi.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/dmamux.hpp>
#include <stmcpp/i2c.hpp>

namespace fastic {
    using namespace stmcpp::units;
    static constexpr uint16_t fasticBufferSize = 1024;

    uint32_t tmpReg, slip, valid, errors;
    uint64_t data;

    // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    __attribute__((section(".dma_buffer"))) uint32_t fastic2_buffers[2][fasticBufferSize];

    aurora::rx fastic2(&fastic2_buffers[0][0], 1000);
    aurora::rx fastic22(&fastic2_buffers[0][1], 1000);

    //uint32_t fastic2_buffers_b[2][bufferSize];
    
    // FastIC1
    //stmcpp::gpio::pin<stmcpp::gpio::port::porte, 3>  fastic1_rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
   // stmcpp::gpio::pin<stmcpp::gpio::port::porte, 4>  fastic1_rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    // I2C3 is used for FastIC1
   // stmcpp::gpio::pin<stmcpp::gpio::port::porta, 8>  fastic1_scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    //stmcpp::gpio::pin<stmcpp::gpio::port::portc, 9>  fastic1_sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    // SPI2 and SPI3 is used for FastIC1
    /*stmcpp::gpio::pin<stmcpp::gpio::port::portb, 14> fastic1_miso_a  (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 9>  fastic1_clk_a   (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 11> fastic1_miso_b  (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 10> fastic1_clk_b   (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);*/
    // Voltage monitoring is available
   // stmcpp::gpio::pin<stmcpp::gpio::port::porta, 7>  fastic1_vmon     (stmcpp::gpio::mode::analog);
    // Set up the I2C peripheral
    //stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c3> fastic1_i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    //stmcpp::i2c::address fastic1_address (0x10);
    // Set up the SPI
    //stmcpp::spi::spi<stmcpp::spi::peripheral::spi2> fastic1_spi_a (stmcpp::spi::role::master, stmcpp::spi::mode::rxSimplex, 32);
    //stmcpp::spi::spi<stmcpp::spi::peripheral::spi3> fastic1_spi_b (stmcpp::spi::role::master, stmcpp::spi::mode::rxSimplex, 32);

    //SPI2 and 3 RX DMA requests reouted to channels 0 and 1
    //stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel0> dmamux1ch0(stmcpp::dmamux1::request::spi2_rx_dma);
    //stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel1> dmamux1ch1(stmcpp::dmamux1::request::spi3_rx_dma);
    //DMA1 stream 0 used for the transfers from FastIC1 SPI A
    //stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream0> fastic1_dma_a(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(SPI2_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&fastic1_buffers_a[0][0], (uint32_t)&fastic1_buffers_a[1][0], bufferSize, stmcpp::dma::priority::veryHigh, false, stmcpp::dma::pincOffset::psize, true);
    //DMA1 stream 1 used for the transfers from FastIC1 SPI B
    //stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream1> fastic1_dma_b(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(SPI3_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&fastic1_buffers_b[0][0], (uint32_t)&fastic1_buffers_b[1][0], bufferSize, stmcpp::dma::priority::veryHigh, false, stmcpp::dma::pincOffset::psize, true);







    // FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 6>  fastic2_rst_n    (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 7>  fastic2_rstcnt_n (stmcpp::gpio::mode::output, stmcpp::gpio::otype::openDrain, stmcpp::gpio::pull::noPull);
    // I2C4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 12> fastic2_scl      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 13> fastic2_sda      (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    // SPI1 and SPI4 is used for FastIC2
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 4>  fastic2_miso_a   (stmcpp::gpio::mode::af5/*, stmcpp::gpio::speed::veryHigh*/);
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 3>  fastic2_clk_a    (stmcpp::gpio::mode::af5/*, stmcpp::gpio::speed::veryHigh*/);
   /* stmcpp::gpio::pin<stmcpp::gpio::port::porte, 5>  fastic2_miso_b   (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 2>  fastic2_clk_b    (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);*/

    //stmcpp::gpio::pin<stmcpp::gpio::port::porta, 6>  fastic2_vmon     (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c4> fastic2_i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    stmcpp::i2c::address fastic2_address (0x10);

    // Set up the SPI
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi1> fastic2_spi (stmcpp::spi::role::slave, stmcpp::spi::mode::rxSimplex, 32, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::msbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
    // Set up the DMA
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel0> dmamux1ch0(stmcpp::dmamux1::request::spi1_rx_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream0> fastic2_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(SPI1_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&fastic2_buffers[0][0], (uint32_t)&fastic2_buffers[1][0], fasticBufferSize, stmcpp::dma::priority::veryHigh, false, stmcpp::dma::pincOffset::psize, true);

    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 11>  fastic1_inj (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 13>  fastic2_inj (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::veryHigh);

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

    
        fastic2_rst_n.set();

        stmcpp::clock::systick::waitBlocking(10_ms);

       // fastic1_i2c.enable();
        fastic2_i2c.enable();

        // Read out the ASIC version to check communication
        //tmpReg = fastic1_i2c.readRegister(0x7f,fastic1_address);

        // Read out the ASIC version to check communication
        tmpReg = fastic2_i2c.readRegister(0x7f, fastic2_address);

        // Reduce the serializer speed to the minimum
        static constexpr uint8_t speed = 0x04;
        static constexpr uint8_t spReg = 0x02 | (speed << 3); 
        fastic2_i2c.writeRegister(0xb9, spReg , fastic2_address);

        tmpReg = fastic2_i2c.readRegister(0x00, fastic2_address);

        fastic2_i2c.writeRegister(0x00, 0, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 1, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 2, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 3, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 4, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 5, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 6, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 7, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 8, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 9, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 10, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 11, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 12, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 13, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 14, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 15, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 16, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 17, fastic2_address);
        fastic2_i2c.writeRegister(0x00, 18, fastic2_address);

         fastic2_i2c.writeRegister(0x00, 0, fastic2_address);

          fastic2_i2c.writeRegister(0x00, 0, fastic2_address);

        
        // Disable scrambling on the aurora bus
        //fastic2_i2c.writeRegister(0x89, 0x00, fastic2_address);


        syncClock();
        
        fastic2_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream0_IRQn);
        fastic2_dma.setNumberOfData(fasticBufferSize);
        fastic2_dma.disableDoubleBuffer();

        fastic2_spi.setNumberOfData(fasticBufferSize);
        fastic2_spi.enableIoSwap();
        fastic2_spi.enableSoftwareSS();
        fastic2_spi.enableRxDma();

        fastic2_dma.enable();
        fastic2_spi.enable();

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


extern "C" void DMA_STR0_IRQHandler(){
    /*for(int i = 0; i < fastic::fasticBufferSize; i++){
        if(fastic::fastic2_buffers[(!fastic::fastic2_dma.getTargetMemory()) & 0x01][i] == 0x123456)
    }*/

   if(!fastic::fastic2.trySync()){
		__ASM volatile("bkpt");
	}else{
        fastic::slip = fastic::fastic2.getBitSlip();
	}
        
    fastic::fastic2.processBuffer();
    auto packets = fastic::fastic2.getPacketBuffer();
    fastic::valid = packets.size();
    fastic::errors = std::count_if(packets.begin(), packets.end(), [](aurora::packet packet) { return (packet.getType() == aurora::packet::type::error); });
    fastic::slip = fastic::fastic2.getBitSlip();

    for (aurora::packet p : packets){
        fastic::data = p.getData();
        __ASM volatile("bkpt");
    }

   __ASM volatile("bkpt");
}
