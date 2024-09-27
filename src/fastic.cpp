#include "stm32h753xx.h"
#include "fastic.hpp"
#include "ad9510.hpp"

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

    uint32_t tmpReg;

    // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    __attribute__((section(".dma_buffer"))) uint32_t fastic2_buffers[2][fasticBufferSize];

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
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi1> fastic2_spi (stmcpp::spi::role::slave, stmcpp::spi::mode::rxSimplex, 32, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::lsbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
    // Set up the DMA
    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel0> dmamux1ch0(stmcpp::dmamux1::request::spi1_rx_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream0> fastic2_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(SPI1_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&fastic2_buffers[0][0], (uint32_t)&fastic2_buffers[1][0], fasticBufferSize, stmcpp::dma::priority::veryHigh, true, stmcpp::dma::pincOffset::psize, true);



       
    void init() {

        //Enable RNG
        RNG->CR |= RNG_CR_RNGEN;

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

        // Serializer word (MSB to LSB)
        fastic2_i2c.writeRegister(0xA3, 0xCC, fastic2_address);
        fastic2_i2c.writeRegister(0xA4, 0xCC, fastic2_address);
        fastic2_i2c.writeRegister(0xA5, 0xCC, fastic2_address);
        fastic2_i2c.writeRegister(0xA6, 0xCC, fastic2_address);
        
        // Force contnuous serializer word
        fastic2_i2c.writeRegister(0x87, (0x3F | 0x80) , fastic2_address);
        //NVIC_EnableIRQ(SPI1_IRQn);

        syncClock();
        
        //fastic1_dma_b.enable();
     //   fastic2_dma.enableInterrupt(stmcpp::dma::interrupt::transferError);
     //   NVIC_EnableIRQ(DMA1_Stream0_IRQn);

        //fastic2_spi_a.setNumberOfData(bufferSize);
       /* fastic2_spi.enableRxDma();
        fastic2_dma.enable();
        fastic2_spi.enable();
        fastic2_spi.startTransfer();*/

    }

    void syncClock() {

      /*  fastic2_spi.enableInterrupt(stmcpp::spi::interrupt::overrun);
        fastic2_spi.enableInterrupt(stmcpp::spi::interrupt::modeFault);
        fastic2_spi.enableInterrupt(stmcpp::spi::interrupt::duplexPacket);*/

        // Force the FastIC+ to output a known serializer word (a square wave in this case)
        // Serializer word (MSB to LSB)
        fastic2_i2c.writeRegister(0xA3, 0x12, fastic2_address);
        fastic2_i2c.writeRegister(0xA4, 0x34, fastic2_address);
        fastic2_i2c.writeRegister(0xA5, 0x56, fastic2_address);
        fastic2_i2c.writeRegister(0xA6, 0x78, fastic2_address);
        // Force contnuous serializer word
        fastic2_i2c.writeRegister(0x87, (0x3F | 0x80) , fastic2_address);

 
       
        ad9510::setChannelDelayCoarse(ad9510::channel::out6, ad9510::rampCaps::c4, ad9510::rampCurrent::u200);
        ad9510::setChannelDelayFine(ad9510::channel::out6, 10);
        ad9510::enableChannelDelay(ad9510::channel::out6);
        stmcpp::clock::systick::waitBlocking(100_ms);

        fastic2_spi.disable();
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

/*
        int hitcounts[delaySteps];
        char * str;

        fastic2_spi.setNumberOfData(fasticBufferSize);
        fastic2_spi.enableIoSwap();
        fastic2_spi.enableSoftwareSS();
        //fastic2_spi.setInternalSs();
        fastic2_spi.enable();
        // Map the clock shift of the selected channel to the probability of correct data reception
            for(uint8_t delay = 0; delay < delaySteps; delay++) {
                // Set the selected channel delay
                ad9510::setChannelDelayFine(ad9510::channel::out6, delay);
                hitcounts[delay] = 0;
                stmcpp::clock::systick::waitBlocking(100_ms);

                // Enable the DMA and start the transfer
                fastic2_spi.enableRxDma();
                fastic2_dma.enable();
                fastic2_spi.setNumberOfData(fasticBufferSize);
                fastic2_spi.enable();
                fastic2_spi.startTransfer();

                // Wait for the transfer to finish
                while (!fastic2_spi.getInterruptFlag(stmcpp::spi::interrupt::endOfTransfer)){;}

                fastic2_spi.disableRxDma();
                fastic2_dma.disable();
                fastic2_spi.disable();

                // Go through the data and check if it corresponds to the selected pattern
                for(int i = 0; i < fasticBufferSize; i++){
                    if(fastic2_buffers[0][i] == 0xAAAAAAAA) hitcounts[delay]++;
                }
                
                //sprintf(str, "Hitcounts on delay %02d are: %04d, that is %03f\n\r", delay, hitcounts[delay], ((float)hitcounts[delay]/(float)fasticBufferSize)*100.0);
                //printf(str);
                __ASM volatile("bkpt");

            }
            
        
*/

        
    }
}

/*
extern "C" void SPI1_IRQHandler(){
    __ASM volatile("bkpt");
}*/