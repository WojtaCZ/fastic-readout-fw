#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include "clock.hpp"
#include "dma.hpp"
#include "bdma.hpp"
#include "gpio.hpp"
#include "dmamux.hpp"
#include "spi.hpp"
#include "i2c.hpp"

uint8_t databuff[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

extern "C" void SystemInit(void){
	//Initialize the clock
	if(clock::init() != 0) {
		__ASM("nop");
	};
	RCC->APB4ENR |= (1 << RCC_APB4ENR_SYSCFGEN_Pos);

	//reg::write<RCC_BASE + offsetof(RCC_TypeDef, APB4ENR)>(1 << RCC_APB4ENR_SYSCFGEN_Pos);


	//Enable the compensation cell
	SYSCFG->CCCSR |= (1 << SYSCFG_CCCSR_EN_Pos);

	while(!(SYSCFG->CCCSR & SYSCFG_CCCSR_READY_Msk)){};

	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;


}


extern "C" int main(void){
	

	//RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;


	//gpio::gpio<gpio::port::porte, 1, gpio::mode::output> ledYellow;
	//gpio::gpio<gpio::port::portb, 14, gpio::mode::output> ledRed;

	
	//NVIC_EnableIRQ(EXTI9_5_IRQn);
	//in.enableInterrupt(gpio::edge::falling);
	
	//Configure SPI GPIO
	gpio::gpio<gpio::port::portb, 5, gpio::mode::af5, gpio::otype::pushpull, gpio::pull::nopull, gpio::speed::high> spi1mosi;
	gpio::gpio<gpio::port::portb, 4, gpio::mode::af5, gpio::otype::pushpull, gpio::pull::nopull, gpio::speed::high> spi1miso;
	gpio::gpio<gpio::port::portb, 3, gpio::mode::af5, gpio::otype::pushpull, gpio::pull::nopull, gpio::speed::high> spi1sck;

	//Enable SPI clock
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	spi::spi<spi::peripheral::spi1, spi::role::master, spi::mode::txsimplex, spi::protocol::motorola, spi::bitorder::msbfirst, spi::clockpol::idlelow, spi::clockphase::firsttransition, spi::ssorigin::sspad, spi::sspol::activelow, true, spi::ssbehavior::endoftransfer, 8, 0, spi::masterdivider::div2> spi1;
	spi1.enable();


	//SPI1 TX DMA request routed to channel 0 of the MUX
	//dmamux1::dmamux<dmamux1::channel::channel0, dmamux1::request::spi1_tx_dma> dmamux1ch0;
	//DMA1 stream 0 used for the transfers from buffer to SPI
	//dma::dma<dma::peripheral::dma1, dma::stream::stream0, dma::mode::mem2periph, dma::datasize::byte, false , dma::datasize::byte, true, 10, dma::priority::veryhigh> dma1s0((uint32_t)&(SPI1->TXDR), (uint32_t)&databuff[0]);
	//Enable the stream
	//dma1s0.enable();

	

	while(1){
		for (int i = 0; i < 320000; i++){
			__ASM("nop");
		}

		//ledRed.write(in.read());
		SPI1->TXDR = 0xAB;
		
		
	}
	
}



extern "C" void HardFault_Handler(void){
	//Ooops, hard fault!
	__ASM("bkpt");

	/*
	What now?
	Check the CFSR
		DIVBYZERO - division by zero
		UNALIGNED - access of unaligned word
		NOCP - disabled coprocessor (is the FPU enabled?)
		INVPC
		INVSTATE - can occur 99% only if writing hand assembly
		UNDEFINSTR - undefined instruction (corrupted stack, wrong code path)

	Check the BFSR
		BFARVALID - indicates that BFAR holds the address causing the fault
		LSPERR & STKERR - might occur in case of stack overflow
		UNSTKERR - fault while returning from except (corrupted stack)
		IMPRECISERR - was the MCU able to determine the exact fault location?
		PRECISERR - instruction executed befor exception caused the fault
	*/
	
}
