#include "stm32h753xx.h"
#include "fastic.hpp"

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
    static constexpr uint16_t bufferSize = 1024;

    uint32_t tmpReg;

    // These buffers need to be placed a memory that is accessible by the DMA
    /// RAM_D2 is chosen for the best possible performance
    __attribute__((section(".dma_buffer"))) uint32_t fastic2_buffers[2][bufferSize];

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
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 4>  fastic2_miso_a   (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 3>  fastic2_clk_a    (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
   /* stmcpp::gpio::pin<stmcpp::gpio::port::porte, 5>  fastic2_miso_b   (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 2>  fastic2_clk_b    (stmcpp::gpio::mode::af5, stmcpp::gpio::speed::veryHigh);*/

    //stmcpp::gpio::pin<stmcpp::gpio::port::porta, 6>  fastic2_vmon     (stmcpp::gpio::mode::af4, stmcpp::gpio::otype::openDrain, stmcpp::gpio::speed::high);
    // Set up the I2C peripheral
    stmcpp::i2c::i2c<stmcpp::i2c::peripheral::i2c4> fastic2_i2c (0x3, 0x4, 0x2, 0xF, 0x13);
    stmcpp::i2c::address fastic2_address (0x10);
    // Set up the SPI
    stmcpp::spi::spi<stmcpp::spi::peripheral::spi1> fastic2_spi_a (stmcpp::spi::role::master, stmcpp::spi::mode::rxSimplex, 32, stmcpp::spi::masterDivider::div2, stmcpp::spi::protocol::motorola, stmcpp::spi::bitOrder::msbFirst, stmcpp::spi::clockPol::idleLow, stmcpp::spi::clockPhase::firstTransition);
    //stmcpp::spi::spi<stmcpp::spi::peripheral::spi4> fastic2_spi_b (stmcpp::spi::role::master, stmcpp::spi::mode::rxSimplex, 32);

    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel0> dmamux1ch0(stmcpp::dmamux1::request::spi1_rx_dma);

    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream0> fastic2_dma_a(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(SPI1_BASE) + offsetof(SPI_TypeDef, RXDR), stmcpp::dma::datasize::word, true, (uint32_t)&fastic2_buffers[0][0], (uint32_t)&fastic2_buffers[1][0], bufferSize, stmcpp::dma::priority::veryHigh, true, stmcpp::dma::pincOffset::psize, true);



       
    void init() {



        /*fastic1_spi_b.setNumberOfData(bufferSize);
        fastic1_spi_b.enableRxDma();
        fastic1_spi_b.enable();*/

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
        fastic2_i2c.writeRegister(0xA3, 0xAA, fastic2_address);
        fastic2_i2c.writeRegister(0xA4, 0xAA, fastic2_address);
        fastic2_i2c.writeRegister(0xA5, 0xAA, fastic2_address);
        fastic2_i2c.writeRegister(0xA6, 0xAA, fastic2_address);
        
        // Force contnuous serializer word
        fastic2_i2c.writeRegister(0x87, (0x3F | 0x80) , fastic2_address);
        
        //fastic1_dma_b.enable();
        fastic2_dma_a.enableInterrupt(stmcpp::dma::interrupt::transferError);
        NVIC_EnableIRQ(DMA1_Stream0_IRQn);

        //fastic2_spi_a.setNumberOfData(bufferSize);
        fastic2_spi_a.enableRxDma();
        fastic2_dma_a.enable();
        fastic2_spi_a.enable();
        fastic2_spi_a.startTransfer();

        

    }
}


