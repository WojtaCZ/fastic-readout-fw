//#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include "clock.hpp"
#include "register.hpp"

#include "gpio.hpp"
#include "usart.hpp"
#include "spi.hpp"
#include "dma.hpp"
#include "dmamux.hpp"

uint8_t databuff[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};



extern "C" void SystemInit(void){
	//Initialize the clock
	clock::init();

	RCC->APB4ENR |= (1 << RCC_APB4ENR_SYSCFGEN_Pos);

	//reg::write<RCC_BASE + offsetof(RCC_TypeDef, APB4ENR)>(1 << RCC_APB4ENR_SYSCFGEN_Pos);


	//Enable the compensation cell
	//SYSCFG->CCCSR |= (1 << SYSCFG_CCCSR_EN_Pos);

	//while(!(SYSCFG->CCCSR & SYSCFG_CCCSR_READY_Msk)){};

	clock::enablePeripherals(
		clock::peripheral::gpiob,
	 	clock::peripheral::gpiod,
		clock::peripheral::gpioa,
		clock::peripheral::gpiod,
		clock::peripheral::spi1,
		clock::peripheral::dma1
	);

	
}


gpio::pin<gpio::port::portb, 14> ledRed(gpio::mode::output);

// USART2
gpio::pin<gpio::port::porta, 5> spiSck(gpio::mode::af5);
gpio::pin<gpio::port::portb, 5> spiMosi(gpio::mode::af5);
gpio::pin<gpio::port::porta, 6> spiMiso(gpio::mode::af5);

extern "C" int main(void){

	dmamux1::dmamux<dmamux1::channel::channel0> dmamux1ch0(dmamux1::request::spi1_tx_dma);

	dma::dma<dma::peripheral::dma1, dma::stream::stream0> dma1s0(dma::mode::mem2periph, dma::datasize::byte, false, (std::uint32_t)(&SPI1->TXDR), dma::datasize::byte, true, (std::uint32_t)(&databuff[0]), (std::uint32_t)(&databuff[0]), 8);

	spi::spi<spi::peripheral::spi1> spi1(spi::role::master, spi::mode::txsimplex, 8, spi::masterdivider::div256);
	spi1.setNumberOfData(8);
	spi1.enableTxDma();
	spi1.enable();

	dma1s0.enable();
	spi1.startTransfer();

	while(1){
		for (int i = 0; i < 320000; i++){
			__ASM("nop");
		}
		
		
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
